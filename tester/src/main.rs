use rand::prelude::*;
use std::fs::{remove_file, OpenOptions};
use std::io::{self, prelude::*, BufWriter};
use std::path::Path;
use std::process::Command;
use structopt::StructOpt;

fn create_random_files<P1: AsRef<Path>, P2: AsRef<Path>>(
    input_path: P1,
    output_path: P2,
    length: usize,
    (line_length_lower, line_length_upper): (usize, usize),
    prob_dup: f32,
    prob_del: f32,
    prob_sub: f32,
) -> Result<(), io::Error> {
    let mut input = BufWriter::new(
        OpenOptions::new()
            .create(true)
            .write(true)
            .open(input_path)?,
    );
    let mut output = BufWriter::new(
        OpenOptions::new()
            .create(true)
            .write(true)
            .open(output_path)?,
    );
    for _ in 0..length {
        let chr = thread_rng().gen_range('a' as u8, 'z' as u8);
        let len: usize = thread_rng().gen_range(line_length_lower, line_length_upper);
        input.write(&vec![chr; len])?;
        input.write(b"\n")?;
        let dup: f32 = thread_rng().gen_range(0., 1.);
        if dup < prob_dup {
            output.write(&vec![chr; len])?;
            output.write(b"\n")?;
            output.write(&vec![chr; len])?;
            output.write(b"\n")?;
        } else {
            let sub: f32 = thread_rng().gen_range(0., 1.);
            if sub < prob_sub {
                output.write(&vec![chr; len])?;
                output.write(b"\n")?;
            } else {
                let del: f32 = thread_rng().gen_range(0., 1.);
                if del > prob_del {
                    output.write(&vec![chr; len])?;
                    output.write(b"\n")?;
                }
            }
        }
    }
    Ok(())
}

#[derive(StructOpt)]
pub struct Config {
    #[structopt(long, short)]
    patcher: std::path::PathBuf,
    #[structopt(long, short)]
    result: std::path::PathBuf,
    #[structopt(long, short)]
    start_length: usize,
    #[structopt(long, short)]
    end_length: usize,
    #[structopt(long, short)]
    amount_of_points: usize,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let config = Config::from_args();
    let mut output = csv::Writer::from_path(&config.result).expect("csv_writer");
    output.write_record(&["Length", "Time"])?;
    let repeat = (config.end_length - config.start_length) / config.amount_of_points;

    create_random_files("input_bench", "output_bench", 0, (5, 30), 0.1, 0.1, 0.1)
        .expect("create input_bench");

    let mut command = Command::new(&config.patcher);
    command.arg("input_bench").arg("output_bench").arg("patch");

    for i in 0..config.amount_of_points {
        let length = config.start_length + i * repeat;
        println!("Running run {} of length {}", i, length);
        remove_file("input_bench")?;
        remove_file("output_bench")?;
        create_random_files("input_bench", "output_bench", length, (5, 30), 0.1, 0.1, 0.1)
            .expect("create input_bench");
        println!("Created files");

        let start = std::time::Instant::now();
        command.output()?;
        let end = std::time::Instant::now();
        let elapsed = end.duration_since(start).as_secs_f32();
        output.write_record(&[format!("{}", length), format!("{}", elapsed)])?;
    }
    Ok(())
}
