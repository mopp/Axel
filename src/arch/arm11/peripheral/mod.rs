mod addr;
mod gpio;

pub fn init()
{
    gpio::set_pin_function(gpio::GpioPin::OkLed, gpio::Function::Output);

    loop {
        gpio::write_output_pin(gpio::GpioPin::OkLed, gpio::Output::CLEAR);
        gpio::dummy_wait();
        gpio::write_output_pin(gpio::GpioPin::OkLed, gpio::Output::SET);
        gpio::dummy_wait();
    }
}
