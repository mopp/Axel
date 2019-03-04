use super::super::thread;
use super::handler::InterruptFrame;
use super::table::InterruptDescriptorTable;
use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::port::Port;

lazy_static! {
    static ref MASTER_COMMAND_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0x20));
    static ref MASTER_DATA_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0x21));
    static ref SLAVE_COMMAND_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0xA0));
    static ref SLAVE_DATA_PORT: Mutex<Port<u8>> = Mutex::new(Port::new(0xA1));
}

const IRQ_OFFSET_MASTER: u8 = 0x20;
const IRQ_OFFSET_SLAVE: u8 = IRQ_OFFSET_MASTER + 8;
const IRQ_NUMBER_TIMER: u8 = IRQ_OFFSET_MASTER;

const END_OF_INTERRUPT: u8 = 0x20;
const ICW1_INIT: u8 = 0x10;
const ICW1_ICW4_REQUIRED: u8 = 0x01;
const ICW4_8086_MODE: u8 = 0x01;

pub fn init() {
    unsafe {
        // Initialize PICs.
        // 1. Send initialize command
        MASTER_COMMAND_PORT.lock().write(ICW1_INIT | ICW1_ICW4_REQUIRED);
        io_wait();
        MASTER_COMMAND_PORT.lock().write(ICW1_INIT | ICW1_ICW4_REQUIRED);
        io_wait();

        // 2. Send vector offset
        MASTER_DATA_PORT.lock().write(IRQ_OFFSET_MASTER);
        io_wait();
        SLAVE_DATA_PORT.lock().write(IRQ_OFFSET_SLAVE);
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

        // Disable all interrupts.
        MASTER_DATA_PORT.lock().write(0b1111_1111);
        SLAVE_DATA_PORT.lock().write(0b1111_1111);
    }
}

pub fn set_handlers(idt: &mut InterruptDescriptorTable) {
    idt.set_handler(IRQ_NUMBER_TIMER, timer_handler);
}

/// If the IRQ came from the slave PIC, it is necessary to issue end of interrupt to both PICs.
fn send_end_of_interrupt(irq: u8) {
    if IRQ_OFFSET_SLAVE <= irq {
        unsafe { SLAVE_COMMAND_PORT.lock().write(END_OF_INTERRUPT) };
    }

    unsafe { MASTER_COMMAND_PORT.lock().write(END_OF_INTERRUPT) };
}

pub unsafe fn enable_timer_interrupt() {
    // FIXME
    MASTER_DATA_PORT.lock().write(0b1111_1110);
}

pub extern "x86-interrupt" fn timer_handler(frame: &mut InterruptFrame) {
    // FIXME: use listener pattern.
    thread::switch_context(frame);

    send_end_of_interrupt(IRQ_NUMBER_TIMER);
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
