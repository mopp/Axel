struct Process {
}

const MAX_PROCESS_COUNT: usize = 255;

pub struct ProcessManager {
    processes: [Process; MAX_PROCESS_COUNT],
}
