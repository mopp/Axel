mod addr;
mod gpio;
mod spi;
mod timer;

pub fn init()
{
    spi::init_spi0();

    // Blinking LED.
    gpio::set_pin_function(gpio::Pin::OkLed, gpio::Function::Output);
    loop {
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Clear);
        timer::wait(500);
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Set);
        timer::wait(500);
    }
}
