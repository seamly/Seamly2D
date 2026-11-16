// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief The Iced window: inputs, layout, export format, and results.
//!
//! Workflow:
//!   1. Inputs: Seamly2D program, pattern, measurements, settings, basename, output folder.
//!      Next runs `seamly2d --layout-info`; an oversized piece selects Split the pieces.
//!   2. Layout (multisize only): the views of the report; split runs list Marker and Nested as unavailable.
//!   3. Export format: formats every material allows for the view, the others with a reason;
//!      the command of each material, ready to copy.
//!   4. Results: per material exit code, file check and messages; the ZIP when every material passed.
//! Long steps run on a worker thread, so the window stays responsive.

use std::future::Future;
use std::path::{Path, PathBuf};

use iced::widget::{button, column, container, horizontal_space, pick_list, row, scrollable, text, text_input, Column};
use iced::{Element, Length, Task};

use crate::cli_command::{DxfVariant, ExportRequest};
use crate::export_run::{plan_session, run_session, PlannedRun, SessionResult};
use crate::layout_report::{query_report, LayoutReport};
use crate::output_naming::session_stamp;
use crate::preferences_layout_folder::layout_folder;

/// @brief Open the window.
/// @param arguments Program arguments: `--seamly2d`, `--pattern`, `--mfile`, `--settings`, `--basename` and
///        `--output-dir` fill the input fields; `--next` reads the pattern at once.
pub fn run(arguments: Vec<String>) -> iced::Result {
    iced::application("Seamly2D CLI Tester", App::update, App::view)
        .window_size(iced::Size::new(1040.0, 800.0))
        .run_with(move || App::new(&arguments))
} // fn run

/// @brief Write the DXF export options of `variant` to a file in the temporary folder.
/// @return Path of the options file.
pub fn write_dxf_options(variant: DxfVariant) -> Result<PathBuf, String> {
    let path = std::env::temp_dir().join(format!("cli_test_app_dxf_{}.json", std::process::id()));
    std::fs::write(&path, variant.options_json()).map_err(|error| format!("{} cannot be written: {error}", path.display()))?;
    Ok(path)
} // fn write_dxf_options

/// @brief Run blocking work on its own thread and await its result.
fn on_worker<T: Send + 'static>(work: impl FnOnce() -> T + Send + 'static) -> impl Future<Output = T> {
    let (sender, receiver) = iced::futures::channel::oneshot::channel();
    std::thread::spawn(move || {
        let _ = sender.send(work());
    });
    async move { receiver.await.expect("the worker thread ended without a result") }
} // fn on_worker

/// @brief Screens in workflow order.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Screen {
    Inputs,
    Layout,
    Format,
    Results,
}

/// @brief File fields with a Browse button.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Field {
    Seamly2d,
    Pattern,
    Measurements,
    Settings,
    OutputFolder,
}

#[derive(Clone, Debug)]
enum Message {
    Edited(Field, String),
    BasenameEdited(String),
    Browse(Field),
    Picked(Field, Option<PathBuf>),
    Next,
    ReportLoaded(Result<LayoutReport, String>),
    ViewSelected(String),
    ToFormat,
    FormatSelected(String),
    TextModeSelected(String),
    DxfSelected(DxfVariant),
    Back,
    Run,
    SessionDone(Result<SessionResult, String>),
    Copy(String),
    StartOver,
}

struct App {
    screen: Screen,
    seamly2d: String,
    pattern: String,
    measurements: String,
    settings: String,
    basename: String,
    output_folder: String,
    busy: Option<String>,
    error: Option<String>,
    report: Option<LayoutReport>,
    split: bool,
    view: Option<String>,
    format: Option<String>,
    svg_text_mode: Option<String>,
    dxf_variant: DxfVariant,
    stamp: String,
    plan: Vec<PlannedRun>,
    result: Option<SessionResult>,
}

impl App {
    fn new(arguments: &[String]) -> (Self, Task<Message>) {
        let value = |name: &str| crate::option_value(arguments, name);
        let pattern = value("--pattern").unwrap_or_default();
        let stem = Path::new(&pattern).file_stem().map(|s| s.to_string_lossy().to_string()).unwrap_or_default();
        let app = App {
            screen: Screen::Inputs,
            seamly2d: value("--seamly2d").unwrap_or_else(|| crate::default_seamly2d().display().to_string()),
            pattern,
            measurements: value("--mfile").unwrap_or_default(),
            settings: value("--settings").unwrap_or_default(),
            basename: value("--basename").unwrap_or(stem),
            output_folder: value("--output-dir").unwrap_or_else(|| layout_folder().display().to_string()),
            busy: None,
            error: None,
            report: None,
            split: false,
            view: None,
            format: None,
            svg_text_mode: None,
            dxf_variant: DxfVariant::AstmR13,
            stamp: String::new(),
            plan: Vec::new(),
            result: None,
        };
        let start = if arguments.iter().any(|argument| argument == "--next") {
            Task::done(Message::Next)
        } else {
            Task::none()
        }; // start
        (app, start)
    } // fn new

    /// @brief Optional path text: None when blank.
    fn optional(text: &str) -> Option<PathBuf> {
        let trimmed = text.trim();
        (!trimmed.is_empty()).then(|| PathBuf::from(trimmed))
    } // fn optional

    /// @brief The request of the current choices; the DXF options file is written at Run.
    fn request(&self, export_options: Option<PathBuf>) -> ExportRequest {
        ExportRequest {
            seamly2d: PathBuf::from(self.seamly2d.trim()),
            pattern: PathBuf::from(self.pattern.trim()),
            measurements: Self::optional(&self.measurements),
            settings: Self::optional(&self.settings),
            view: self.view.clone(),
            split: self.split,
            format: self.format.clone().unwrap_or_default(),
            svg_text_mode: self.svg_text_mode.clone(),
            export_options,
            several_materials: self.report.as_ref().is_some_and(|report| report.materials.len() > 1),
        }
    } // fn request

    /// @brief Rebuild the commands shown on the format screen; one stamp per session.
    fn refresh_plan(&mut self) {
        self.stamp = session_stamp(&chrono::Local::now());
        let placeholder = (self.format.as_deref() == Some("dxf")).then(|| std::env::temp_dir().join("cli_test_app_dxf.json"));
        self.plan = match (&self.report, &self.format) {
            (Some(report), Some(_)) => {
                plan_session(&self.request(placeholder), report, Path::new(self.output_folder.trim()), self.basename.trim(), &self.stamp)
            } // report and format
            _ => Vec::new(),
        }; // plan
    } // fn refresh_plan

    /// @brief Choose the first format the view allows and refresh the commands.
    fn choose_default_format(&mut self) {
        let formats = self.formats();
        if !self.format.as_ref().is_some_and(|format| formats.contains(format)) {
            self.format = formats.first().cloned();
        } // if the chosen format is not offered
        self.refresh_plan();
    } // fn choose_default_format

    fn formats(&self) -> Vec<String> {
        self.report.as_ref().map(|report| report.format_choices(self.view.as_deref().unwrap_or(""), self.split)).unwrap_or_default()
    } // fn formats

    fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Edited(field, value) => {
                if field == Field::Pattern {
                    self.default_basename(&value);
                } // if pattern
                *self.field_mut(field) = value;
            } // Edited
            Message::BasenameEdited(value) => self.basename = value,
            Message::Browse(field) => {
                let start = PathBuf::from(self.field(field).trim());
                return Task::perform(on_worker(move || pick(field, &start)), move |path| Message::Picked(field, path));
            } // Browse
            Message::Picked(field, Some(path)) => {
                let value = path.display().to_string();
                if field == Field::Pattern {
                    self.default_basename(&value);
                } // if pattern
                *self.field_mut(field) = value;
            } // Picked
            Message::Picked(_, None) => {} // dialog cancelled
            Message::Next => {
                self.error = None;
                if !Path::new(self.pattern.trim()).is_file() {
                    self.error = Some("Choose an existing pattern file (.sm2d).".to_string());
                    return Task::none();
                } // if no pattern
                self.busy = Some("Reading the pattern and the layout settings...".to_string());
                let (seamly2d, pattern) = (PathBuf::from(self.seamly2d.trim()), PathBuf::from(self.pattern.trim()));
                let (measurements, settings) = (Self::optional(&self.measurements), Self::optional(&self.settings));
                return Task::perform(
                    on_worker(move || query_report(&seamly2d, &pattern, measurements.as_deref(), settings.as_deref())),
                    Message::ReportLoaded,
                );
            } // Next
            Message::ReportLoaded(Ok(report)) => {
                self.busy = None;
                // A piece wider than the media cannot stay whole: split it without asking.
                self.split = report.needs_split();
                self.view = report.multisize.then(|| report.view_choices(self.split).first().cloned()).flatten();
                self.svg_text_mode = report.svg_text_modes.first().cloned();
                self.screen = if report.multisize { Screen::Layout } else { Screen::Format };
                self.report = Some(report);
                self.choose_default_format();
            } // ReportLoaded Ok
            Message::ReportLoaded(Err(error)) => {
                self.busy = None;
                self.error = Some(error);
            } // ReportLoaded Err
            Message::ViewSelected(view) => {
                self.view = Some(view);
                self.choose_default_format();
            } // ViewSelected
            Message::ToFormat => {
                self.screen = Screen::Format;
                self.choose_default_format();
            } // ToFormat
            Message::FormatSelected(format) => {
                self.format = Some(format);
                self.refresh_plan();
            } // FormatSelected
            Message::TextModeSelected(mode) => {
                self.svg_text_mode = Some(mode);
                self.refresh_plan();
            } // TextModeSelected
            Message::DxfSelected(variant) => self.dxf_variant = variant,
            Message::Back => {
                self.error = None;
                self.screen = match self.screen {
                    Screen::Results => Screen::Format,
                    Screen::Format if self.report.as_ref().is_some_and(|report| report.multisize) => Screen::Layout,
                    _ => Screen::Inputs,
                }; // previous screen
            } // Back
            Message::Run => {
                self.error = None;
                let output_folder = PathBuf::from(self.output_folder.trim());
                if let Err(error) = std::fs::create_dir_all(&output_folder) {
                    self.error = Some(format!("{} cannot be created: {error}", output_folder.display()));
                    return Task::none();
                } // if output folder cannot be made
                let export_options = if self.format.as_deref() == Some("dxf") {
                    match write_dxf_options(self.dxf_variant) {
                        Ok(path) => Some(path),
                        Err(error) => {
                            self.error = Some(error);
                            return Task::none();
                        } // Err
                    } // match options file
                } else {
                    None
                }; // export_options
                let request = self.request(export_options);
                let Some(report) = self.report.clone() else { return Task::none(); };
                let (basename, stamp) = (self.basename.trim().to_string(), self.stamp.clone());
                self.busy = Some(format!("Exporting {} material(s)...", report.materials.len()));
                return Task::perform(
                    on_worker(move || Ok(run_session(&request, &report, &output_folder, &basename, &stamp))),
                    Message::SessionDone,
                );
            } // Run
            Message::SessionDone(result) => {
                self.busy = None;
                match result {
                    Ok(result) => {
                        self.result = Some(result);
                        self.screen = Screen::Results;
                    } // Ok
                    Err(error) => self.error = Some(error),
                } // match result
                self.refresh_plan(); // a new session gets a new stamp and free names
            } // SessionDone
            Message::Copy(command) => return iced::clipboard::write(command),
            Message::StartOver => {
                self.report = None;
                self.result = None;
                self.screen = Screen::Inputs;
            } // StartOver
        } // match message
        Task::none()
    } // fn update

    /// @brief Basename follows the pattern name until the user edits it.
    fn default_basename(&mut self, new_pattern: &str) {
        let stem = |path: &str| Path::new(path).file_stem().map(|s| s.to_string_lossy().to_string()).unwrap_or_default();
        if self.basename.trim().is_empty() || self.basename == stem(&self.pattern) {
            self.basename = stem(new_pattern);
        } // if the basename was not edited
    } // fn default_basename

    fn field(&self, field: Field) -> &String {
        match field {
            Field::Seamly2d => &self.seamly2d,
            Field::Pattern => &self.pattern,
            Field::Measurements => &self.measurements,
            Field::Settings => &self.settings,
            Field::OutputFolder => &self.output_folder,
        } // match field
    } // fn field

    fn field_mut(&mut self, field: Field) -> &mut String {
        match field {
            Field::Seamly2d => &mut self.seamly2d,
            Field::Pattern => &mut self.pattern,
            Field::Measurements => &mut self.measurements,
            Field::Settings => &mut self.settings,
            Field::OutputFolder => &mut self.output_folder,
        } // match field
    } // fn field_mut

    fn view(&self) -> Element<'_, Message> {
        let title = match self.screen {
            Screen::Inputs => "1. Inputs and layout settings",
            Screen::Layout => "2. Layout",
            Screen::Format => "3. Export format",
            Screen::Results => "Results",
        }; // title
        let body: Element<'_, Message> = match self.screen {
            Screen::Inputs => self.inputs_view(),
            Screen::Layout => self.layout_view(),
            Screen::Format => self.format_view(),
            Screen::Results => self.results_view(),
        }; // body
        let mut page = column![text(title).size(22), body].spacing(14).padding(18);
        if let Some(busy) = &self.busy {
            page = page.push(text(busy.clone()));
        } // if a run is in progress
        if let Some(error) = &self.error {
            page = page.push(text(error.clone()).style(text::danger));
        } // if an error
        scrollable(container(page).width(Length::Fill)).into()
    } // fn view

    /// @brief One labelled path field with a Browse button.
    fn path_row(&self, label: &'static str, field: Field, placeholder: &'static str) -> Element<'_, Message> {
        row![
            text(label).width(Length::Fixed(160.0)),
            text_input(placeholder, self.field(field)).on_input(move |value| Message::Edited(field, value)),
            button("Browse").on_press_maybe(self.busy.is_none().then_some(Message::Browse(field))),
        ]
        .spacing(8)
        .into()
    } // fn path_row

    fn inputs_view(&self) -> Element<'_, Message> {
        column![
            self.path_row("Seamly2D program", Field::Seamly2d, "seamly2d.exe"),
            self.path_row("Pattern", Field::Pattern, "pattern.sm2d"),
            self.path_row("Measurements", Field::Measurements, "optional: .smis or .smms, overrides the pattern's file"),
            self.path_row("Layout settings", Field::Settings, "optional: settings JSON; empty uses SeamlyLayout's saved settings"),
            row![
                text("Output basename").width(Length::Fixed(160.0)),
                text_input("pattern basename", &self.basename).on_input(Message::BasenameEdited),
            ]
            .spacing(8),
            self.path_row("Output folder", Field::OutputFolder, "Preferences > Paths > Layout"),
            row![horizontal_space(), button("Next").on_press_maybe(self.busy.is_none().then_some(Message::Next))],
        ]
        .spacing(10)
        .into()
    } // fn inputs_view

    /// @brief Facts of the report that decided the choices.
    fn report_summary(&self) -> Element<'_, Message> {
        let Some(report) = &self.report else { return column![].into(); };
        let materials: Vec<String> = report.materials.iter().map(|material| {
            if material.name.is_empty() { "(no material name)".to_string() } else { material.name.clone() }
        }).collect();
        let mut summary = column![
            text(format!("Materials: {}", materials.join(", "))),
            text(format!("Paper: {}", if report.tiled_paper { "tiled pages" } else { "roll or fabric" })),
        ]
        .spacing(4);
        if self.split {
            let pieces: Vec<String> = report.materials.iter()
                .flat_map(|material| material.oversized_pieces.iter().map(|piece| piece.name.clone()))
                .collect();
            // A multisize pattern names each size of a piece; the first names are enough to read.
            const SHOWN: usize = 6;
            let more = pieces.len().saturating_sub(SHOWN);
            let names = pieces.iter().take(SHOWN).cloned().collect::<Vec<_>>().join(", ");
            let rest = if more > 0 { format!(" and {more} more") } else { String::new() };
            summary = summary.push(text(format!("Split the pieces: on. Wider than the media: {names}{rest}")));
        } // if split
        summary.into()
    } // fn report_summary

    fn layout_view(&self) -> Element<'_, Message> {
        let views = self.report.as_ref().map(|report| report.view_choices(self.split)).unwrap_or_default();
        let mut page = column![
            self.report_summary(),
            row![
                text("Layout").width(Length::Fixed(160.0)),
                pick_list(views, self.view.clone(), Message::ViewSelected),
            ]
            .spacing(8),
        ]
        .spacing(10);
        for item in self.report.as_ref().map(|report| report.unavailable_views(self.split)).unwrap_or_default() {
            page = page.push(text(format!("{}: {}", item.name, item.reason)).style(text::secondary));
        } // for each unavailable view
        page.push(row![button("Back").on_press(Message::Back), horizontal_space(), button("Next").on_press(Message::ToFormat)])
            .into()
    } // fn layout_view

    fn format_view(&self) -> Element<'_, Message> {
        let formats = self.formats();
        let mut page = column![
            self.report_summary(),
            text(format!("Layout: {}", self.view.clone().unwrap_or_else(|| "individual pattern".to_string()))),
            row![
                text("Export format").width(Length::Fixed(160.0)),
                pick_list(formats, self.format.clone(), Message::FormatSelected),
            ]
            .spacing(8),
        ]
        .spacing(10);
        if self.format.as_deref() == Some("svg") {
            let modes = self.report.as_ref().map(|report| report.svg_text_modes.clone()).unwrap_or_default();
            page = page.push(
                row![text("SVG text").width(Length::Fixed(160.0)), pick_list(modes, self.svg_text_mode.clone(), Message::TextModeSelected)]
                    .spacing(8),
            );
        } // if svg
        if self.format.as_deref() == Some("dxf") {
            page = page.push(
                row![
                    text("DXF variant").width(Length::Fixed(160.0)),
                    pick_list(DxfVariant::ALL.to_vec(), Some(self.dxf_variant), Message::DxfSelected),
                ]
                .spacing(8),
            );
        } // if dxf
        let view = self.view.clone().unwrap_or_default();
        for item in self.report.as_ref().map(|report| report.unavailable_formats(&view, self.split)).unwrap_or_default() {
            page = page.push(text(format!("{}: {}", item.name, item.reason)).style(text::secondary));
        } // for each unavailable format
        page = page.push(text("Commands").size(18));
        page = page.push(self.commands_view());
        page.push(
            row![
                button("Back").on_press_maybe(self.busy.is_none().then_some(Message::Back)),
                horizontal_space(),
                button("Run").on_press_maybe((self.busy.is_none() && self.format.is_some()).then_some(Message::Run)),
            ]
            .spacing(8),
        )
        .into()
    } // fn format_view

    fn commands_view(&self) -> Element<'_, Message> {
        let mut list = Column::new().spacing(8);
        for run in &self.plan {
            let material = if run.material.is_empty() { "(no material name)".to_string() } else { run.material.clone() };
            list = list.push(
                column![
                    row![text(material).size(16), horizontal_space(), button("Copy").on_press(Message::Copy(run.command.clone()))],
                    text(run.command.clone()).size(13),
                ]
                .spacing(4),
            );
        } // for each material
        list.into()
    } // fn commands_view

    fn results_view(&self) -> Element<'_, Message> {
        let Some(result) = &self.result else { return column![].into(); };
        let mut page = Column::new().spacing(12).push(text(format!("Session {}", result.stamp)));
        for material in &result.materials {
            let status = if material.succeeded() { "PASS" } else { "FAIL" };
            let name = if material.run.material.is_empty() { "(no material name)" } else { material.run.material.as_str() };
            let detail = match &material.check {
                Ok(bytes) => format!("{} ({bytes} bytes)", material.run.output.display()),
                Err(problem) => problem.clone(),
            }; // detail
            let mut block = column![
                text(format!("{status}  {name}  exit {}", material.exit_code.map_or("-".to_string(), |code| code.to_string())))
                    .size(16)
                    .style(if material.succeeded() { text::success } else { text::danger }),
                text(detail),
                row![text(material.run.command.clone()).size(12), horizontal_space(), button("Copy").on_press(Message::Copy(material.run.command.clone()))],
            ]
            .spacing(4);
            for companion in &material.companions {
                block = block.push(text(format!("also wrote {}", companion.display())));
            } // for each companion file
            for stream in [&material.stdout, &material.stderr] {
                if !stream.trim().is_empty() {
                    block = block.push(text(stream.trim().to_string()).size(12));
                } // if the stream has text
            } // for standard output and error
            page = page.push(block);
        } // for each material
        let archive = match &result.archive {
            Some(Ok(path)) => text(format!("Archive: {}", path.display())).style(text::success),
            Some(Err(error)) => text(format!("Archive failed: {error}")).style(text::danger),
            None => text("No archive: a material failed.").style(text::danger),
        }; // archive
        page.push(archive)
            .push(row![button("Back").on_press(Message::Back), horizontal_space(), button("Start over").on_press(Message::StartOver)])
            .into()
    } // fn results_view
} // impl App

/// @brief File or folder dialog for `field`, starting in the folder of `start`.
fn pick(field: Field, start: &Path) -> Option<PathBuf> {
    let folder = if start.is_dir() { Some(start.to_path_buf()) } else { start.parent().map(Path::to_path_buf) };
    let mut dialog = rfd::FileDialog::new();
    if let Some(folder) = folder.filter(|folder| folder.is_dir()) {
        dialog = dialog.set_directory(folder);
    } // if a start folder exists
    match field {
        Field::Seamly2d if cfg!(windows) => dialog.add_filter("Program", &["exe"]).pick_file(),
        Field::Seamly2d => dialog.pick_file(),
        Field::Pattern => dialog.add_filter("Seamly2D pattern", &["sm2d"]).pick_file(),
        Field::Measurements => dialog.add_filter("Measurements", &["smis", "smms", "vit", "vst"]).pick_file(),
        Field::Settings => dialog.add_filter("Layout settings", &["json"]).pick_file(),
        Field::OutputFolder => dialog.pick_folder(),
    } // match field
} // fn pick
