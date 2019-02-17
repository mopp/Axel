use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::port::Port;

lazy_static! {
    static ref MASTER_COMMAND_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0x20));
    static ref MASTER_DATA_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0x21));
    static ref SLAVE_COMMAND_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0xA0));
    static ref SLAVE_DATA_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0xA1));
}

const END_OF_INTERRUPT: u8 = 0x20;
const ICW1_INIT: u8 = 0x10;
const ICW1_ICW4_REQUIRED: u8 = 0x01;
const ICW4_8086_MODE: u8 = 0x01;

pub fn init() {
    unsafe {
        let mask_master = MASTER_DATA_PORT.lock().read();
        let mask_slave = SLAVE_DATA_PORT.lock().read();

        // Initialize PICs.
        // 1. Send initialize command
        MASTER_COMMAND_PORT.lock().write(ICW1_INIT | ICW1_ICW4_REQUIRED);
        io_wait();
        MASTER_COMMAND_PORT.lock().write(ICW1_INIT | ICW1_ICW4_REQUIRED);
        io_wait();

        // 2. Send vector offset
        // FIXME: use variable.
        MASTER_DATA_PORT.lock().write(0x20);
        io_wait();
        SLAVE_DATA_PORT.lock().write(0x20 + 8);
        io_wait();

        // 3. Send how it is wired to master/slaves
        MASTER_DATA_PORT.lock().write(4);
        io_wait();
        SLAVE_DATA_PORT.lock().write(2);
        io_wait();

        // 4. Gives additional information about the environment.
        MASTER_DATA_PORT.lock().write(ICW4_8086_MODE);
        io_wait();
        SLAVE_DATA_PORT.lock().write(ICW4_8086_MODE);
        io_wait();

        MASTER_DATA_PORT.lock().write(mask_master);
        SLAVE_DATA_PORT.lock().write(mask_slave);
    }
}

pub fn set_interrupt_handlers() {}

/// If the IRQ came from the slave PIC, it is necessary to issue end of interrupt to both PICs.
fn send_end_of_interrupt(irq: u8) {
    if 8 <= irq {
        unsafe { SLAVE_COMMAND_PORT.lock().write(END_OF_INTERRUPT) };
    }

    unsafe { MASTER_COMMAND_PORT.lock().write(END_OF_INTERRUPT) };
}

/// TODO: write document.
fn io_wait() {
    unsafe {
        asm!("outb %al, $$0x80"
             :
             : "al"(0)
             :
             :
        );
    }
}
