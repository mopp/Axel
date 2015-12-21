mod addr;
mod gpio;
mod spi;

pub fn init()
{
    spi::init_spi0();

    // Blinking LED.
    gpio::set_pin_function(gpio::Pin::OkLed, gpio::Function::Output);
    loop {
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Clear);
        gpio::dummy_wait();
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Set);
        gpio::dummy_wait();
    }
}
