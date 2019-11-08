use std::{
    collections::HashMap, fs::OpenOptions, io::prelude::*, path::PathBuf, process::Command,
    time::Instant,
};
use structopt::StructOpt;

#[derive(StructOpt)]
struct Config {
    #[structopt(long, short)]
    command: PathBuf,
    #[structopt(long, short)]
    benchmark_desc: PathBuf,
    #[structopt(long, short)]
    repeat: usize,
    #[structopt(long, short)]
    output: PathBuf,
}

#[derive(serde::Deserialize, Debug)]
struct Benchmark {
    source: PathBuf,
    target: PathBuf,
}

fn main() {
    let config = Config::from_args();
    let mut reader = OpenOptions::new()
        .read(true)
        .open(&config.benchmark_desc)
        .expect("benchmark description");
    let mut bench_desc = String::new();
    reader
        .read_to_string(&mut bench_desc)
        .expect("invalid file");
    let benchmarks: HashMap<String, Benchmark> =
        toml::from_str(&bench_desc).expect("benchmark toml");
    let benchmarks: Vec<(String, Benchmark)> = benchmarks.into_iter().collect();

    let mut csv_output = csv::WriterBuilder::new()
        .from_path(config.output)
        .expect("invalid output");
    let s = "runs".to_string();
    csv_output
        .write_record(std::iter::once(&s).chain(benchmarks.iter().map(|(name, _)| name)))
        .expect("header");
    let mut benchs = Vec::with_capacity(benchmarks.len());
    for i in 0..config.repeat {
        println!("Run {}", i);
        benchs.clear();
        benchs.push(format!("run {}", i));
        for (name, benchmark) in &benchmarks {
            println!("    - Benchmark: {}", name);
            let mut command = Command::new(&config.command);
            let start = Instant::now();
            command
                .arg(&benchmark.source)
                .arg(&benchmark.target)
                .arg("/dev/null")
                .output()
                .expect("invalid command");
            benchs.push(format!(
                "{}",
                Instant::now().duration_since(start).as_secs_f64()
            ));
        }
        csv_output.write_record(&benchs).expect("writing run");
    }
}
