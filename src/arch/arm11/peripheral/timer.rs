use super::addr::Addr;


pub fn wait_usec(usec: u64)
{
    let time_begin = get_time_counter_value();
    loop {
        let time_current = get_time_counter_value();
        if usec <= (time_current - time_begin)  {
            break;
        }
    }
}


pub fn wait(msec: u64)
{
    wait_usec(msec * 1000);
}



fn get_time_counter_value() -> u64
{
    let lo         = Addr::TimerClo.load::<u32>() as u64;
    let hi         = Addr::TimerChi.load::<u32>() as u64;
    (hi << 32) | lo
}
