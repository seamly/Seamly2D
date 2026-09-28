// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file log_sink.rs
// @brief Forwards Rust debug log lines to the C++ Logger, the only writer of the log file.
//
// The C++ Logger owns the log file and serializes every write with a mutex.
// Rust never opens the file: it hands each message to a sink function that
// main() registers once at startup. See seamlylayout_SEAMLYLAYOUT_DECISIONS.md.
//
// The crate does not link against the C++ Logger. The sink is a run-time
// function pointer, so `cargo test` and the C++ test executables each link
// without the other side.

use std::sync::OnceLock;

/// @brief C ABI of the log sink: a UTF-8 message given as pointer and byte length.
///
/// The message is not NUL-terminated, so a message that contains NUL stays whole.
/// The sink must copy the bytes before it returns; the pointer is valid only for the call.
pub type LogSink = unsafe extern "C" fn(text: *const u8, length: usize);

// The registered sink. Set once per process; later registrations are ignored.
static LOG_SINK: OnceLock<LogSink> = OnceLock::new();

/// @brief Registers the function that receives every Rust debug log line.
///
/// @param sink  Function that writes one message to the log file. It must be
///              safe to call from any thread and for the rest of the process.
/// @return true when this call registered the sink; false when a sink was
///         already registered or `sink` is null.
///
/// # Safety
/// `sink` must be null or a valid function with the `LogSink` signature.
#[no_mangle]
pub unsafe extern "C" fn seamly_layout_set_log_sink(sink: Option<LogSink>) -> bool {
    // A null pointer arrives as None; there is nothing to register.
    let Some(sink) = sink else { return false };
    LOG_SINK.set(sink).is_ok()
} // fn seamly_layout_set_log_sink

/// @brief Sends one debug message to the C++ Logger, which adds the
/// `[unix_seconds] DEBUG: ` prefix and writes the line.
///
/// Drops the message when no sink is registered, for example under `cargo test`.
#[cfg(debug_assertions)]
pub(crate) fn log_to_file(message: &str) {
    let Some(sink) = LOG_SINK.get() else { return };
    // Safety: the sink contract requires it to copy the bytes during the call,
    // and `message` outlives the call.
    unsafe { sink(message.as_ptr(), message.len()) };
} // fn log_to_file (debug build)

/// @brief No-op when debug_assertions is disabled: release builds send no Rust log lines.
#[cfg(not(debug_assertions))]
#[inline(always)]
pub(crate) fn log_to_file(_message: &str) {} // fn log_to_file (release build)

#[cfg(test)]
mod tests {
    use super::{log_to_file, seamly_layout_set_log_sink};
    use std::sync::Mutex;

    // Every test in the crate shares the one process-wide sink, so other tests'
    // lines also arrive here. Each test filters on its own marker.
    static RECEIVED: Mutex<Vec<String>> = Mutex::new(Vec::new());

    // @brief Test sink: copies each message into RECEIVED.
    unsafe extern "C" fn collect(text: *const u8, length: usize) {
        let bytes = std::slice::from_raw_parts(text, length);
        let line = String::from_utf8_lossy(bytes).into_owned();
        RECEIVED.lock().unwrap().push(line);
    } // fn collect

    // @brief Registers the test sink; a second registration from another test is harmless.
    fn install_collector() {
        unsafe { seamly_layout_set_log_sink(Some(collect)) };
    } // fn install_collector

    // @brief Returns the received lines that contain `marker`, in arrival order.
    fn lines_with(marker: &str) -> Vec<String> {
        RECEIVED
            .lock()
            .unwrap()
            .iter()
            .filter(|line| line.contains(marker))
            .cloned()
            .collect()
    } // fn lines_with

    #[test]
    fn null_sink_is_rejected() {
        assert!(!unsafe { seamly_layout_set_log_sink(None) });
    } // null_sink_is_rejected

    #[test]
    fn second_registration_is_ignored() {
        install_collector();
        assert!(!unsafe { seamly_layout_set_log_sink(Some(collect)) });
    } // second_registration_is_ignored

    // Two threads log at the same time. Every line must arrive whole, and each
    // thread's lines must keep their order. The order between threads is not defined.
    #[cfg(debug_assertions)]
    #[test]
    fn concurrent_lines_arrive_whole_and_in_order() {
        install_collector();
        const MARKER: &str = "[log_sink_test_concurrent]";
        const LINES_PER_THREAD: usize = 500;

        let writers: Vec<_> = (0..2)
            .map(|thread_id| {
                std::thread::spawn(move || {
                    for index in 0..LINES_PER_THREAD {
                        log_to_file(&format!("{MARKER} thread={thread_id} index={index} end"));
                    }
                })
            })
            .collect();
        for writer in writers {
            writer.join().unwrap();
        }

        let lines = lines_with(MARKER);
        assert_eq!(lines.len(), 2 * LINES_PER_THREAD);

        // Parse "thread=T index=I end" and check that each thread's index counts up by one.
        let mut next_index = [0usize; 2];
        for line in &lines {
            let fields: Vec<&str> = line.split_whitespace().collect();
            assert_eq!(fields.len(), 4, "clipped line: {line}");
            assert_eq!(fields[3], "end", "clipped line: {line}");
            let thread_id: usize = fields[1].trim_start_matches("thread=").parse().unwrap();
            let index: usize = fields[2].trim_start_matches("index=").parse().unwrap();
            assert_eq!(index, next_index[thread_id], "out of order: {line}");
            next_index[thread_id] += 1;
        }
    } // concurrent_lines_arrive_whole_and_in_order

    #[cfg(debug_assertions)]
    #[test]
    fn message_with_nul_arrives_whole() {
        install_collector();
        log_to_file("[log_sink_test_nul] before\0after");
        assert_eq!(lines_with("[log_sink_test_nul]"), vec!["[log_sink_test_nul] before\0after"]);
    } // message_with_nul_arrives_whole

    // Run with `cargo test --release`: the release stub must not reach the sink.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_build_sends_nothing_to_the_sink() {
        install_collector();
        log_to_file("[log_sink_test_release] discarded");
        assert!(lines_with("[log_sink_test_release]").is_empty());
    } // release_build_sends_nothing_to_the_sink
} // mod tests
