mod addr;
mod gpio;
mod spi;
mod timer;

pub fn init()
{
    spi::init_spi0();

    // Blinking LED.
    gpio::set_pin_function(gpio::Pin::OkLed, gpio::Function::Output);

    let mut switch = true;
    loop {
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Clear);
        timer::wait(500);
        gpio::write_output_pin(gpio::Pin::OkLed, gpio::Output::Set);
        timer::wait(500);

        if switch == true {
            spi::ili9340::draw_girl();
            switch = false;
        } else {
            spi::ili9340::fill_screen(0x0000);
            spi::ili9340::draw_aa();
            switch = true;
        }
    }
}
