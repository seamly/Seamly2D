// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

use std::path::PathBuf;

use anyhow::Result;
use app_core::{load_svg, render_png, save_svg};
use clap::{Parser, Subcommand};

// @brief SeamlyLayout CLI entrypoint.
#[derive(Debug, Parser)]
#[command(name = "seamly-layout", author, version, about = "SVG utilities", long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

// @brief Supported CLI commands.
#[derive(Debug, Subcommand)]
enum Commands {
    // Render an SVG to PNG.
    Render {
        // Input SVG path.
        #[arg(short, long)]
        input: PathBuf,
        // Output PNG path.
        #[arg(short, long)]
        output: PathBuf,
        // Scale factor relative to the SVG size (default: 1.0).
        #[arg(short, long, default_value_t = 1.0)]
        scale: f32,
    },
    // Prefix all ids in the SVG and save.
    PrefixIds {
        // Input SVG path.
        #[arg(short, long)]
        input: PathBuf,
        // Output SVG path.
        #[arg(short, long)]
        output: PathBuf,
        // Prefix to add to ids.
        #[arg(short, long)]
        prefix: String,
    },
    // Show basic info about an SVG (width/height).
    Info {
        // Input SVG path.
        #[arg(short, long)]
        input: PathBuf,
    },
}

// @brief Entry point.
fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Commands::Render {
            input,
            output,
            scale,
        } => {
            let (_, tree) = load_svg(&input)?;
            let s = scale.max(0.01);
            render_png(&tree, &output, s)?;
            println!(
                "Rendered {} -> {} (scale {:.2})",
                input.display(),
                output.display(),
                s
            );
        }
        Commands::PrefixIds {
            input,
            output,
            prefix,
        } => {
            let (mut doc, _tree) = load_svg(&input)?;
            doc.ensure_id_prefixed(&prefix);
            save_svg(&doc, &output)?;
            println!(
                "Prefixed ids with '{}' and saved to {}",
                prefix,
                output.display()
            );
        }
        Commands::Info { input } => {
            let (_, tree) = load_svg(&input)?;
            let size = tree.size();
            println!(
                "SVG: {} (width: {:.2}, height: {:.2})",
                input.display(),
                size.width(),
                size.height()
            );
        }
    }
    Ok(())
} // main()
