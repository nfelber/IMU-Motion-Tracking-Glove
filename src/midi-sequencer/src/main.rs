extern crate midir;
extern crate mtg_drivers;

use std::f32::consts::PI;
use std::thread::sleep;
use std::time::Duration;
use std::io::{stdin, stdout, Write};
use std::error::Error;

use midi_sequencer::{Sequencer, Scale, GestureController};
use midir::{MidiOutput, MidiOutputPort};
use uuid::Uuid;
use mtg_drivers::glove;
use cgmath::{Quaternion, Euler, Deg, InnerSpace};

const GLOVE_NAME: &str = "MTG";
const DATA_UUID: Uuid = Uuid::from_u128(0xdc931335_7019_4096_b1e7_42a29e570f8f);

const SCALES: [&[u8]; 5] = [
    &[2, 1, 2, 1, 2, 1, 2, 1], // diminished
    &[2, 1, 4, 1, 4], // hirajoshi
    &[1, 4, 1, 4, 2], // iwato
    &[1, 3, 1, 1, 2, 3, 1], // persian
    &[2, 3, 2, 3, 2] // egyptian
];

#[tokio::main]
async fn main() -> Result<(), ()> {
    match run() {
        Ok(_) => (),
        Err(err) => println!("Error: {}", err)
    };
    Ok(())
}

fn run() -> Result<(), Box<dyn Error>> {

    // TEST
    // let euler: [f32; 3] = [30.0, 0.0, 0.0];
    // let rot_axis: [f32; 3] = [1.0, 0.0, 1.0];
    // let q: Quaternion<f32> = Quaternion::from(Euler {
    //     x: Deg(euler[0]),
    //     y: Deg(euler[1]),
    //     z: Deg(euler[2]),
    // });
    // let ra = [q.v.x, q.v.y, q.v.y]; // rotation axis
    // let scaling = (ra[0] * rot_axis[0] + ra[1] * rot_axis[1] + ra[2] * rot_axis[2]) /
    //     (rot_axis[0] * rot_axis[0] + rot_axis[1] * rot_axis[1] + rot_axis[2] * rot_axis[2]);
    // let p = [rot_axis[0] * scaling, rot_axis[1] * scaling, rot_axis[2] * scaling]; // return projection v1 on to v2  (parallel component)
    // let mut twist: Quaternion<f32> = Quaternion::new(q.s, p[0], p[1], p[2]).normalize();
    // if scaling < 0.0 { twist = -twist; }
    // println!("{}", 2.0 * twist.s.acos() / PI * 180.0);
    // END TEST

    let midi_out = MidiOutput::new("My Test Output")?;
    
    // Get an output port (read from console if multiple are available)
    let out_ports = midi_out.ports();
    let out_port: &MidiOutputPort = match out_ports.len() {
        0 => return Err("no output port found".into()),
        1 => {
            println!("Choosing the only available output port: {}", midi_out.port_name(&out_ports[0]).unwrap());
            &out_ports[0]
        },
        _ => {
            println!("\nAvailable output ports:");
            for (i, p) in out_ports.iter().enumerate() {
                println!("{}: {}", i, midi_out.port_name(p).unwrap());
            }
            print!("Please select output port: ");
            stdout().flush()?;
            let mut input = String::new();
            stdin().read_line(&mut input)?;
            out_ports.get(input.trim().parse::<usize>()?)
                     .ok_or("invalid output port selected")?
        }
    };
    
    println!("\nOpening MIDI connection...");
    let conn_out = midi_out.connect(out_port, "midir-test")?;
    println!("MIDI connection opened.");

    // let base = 36;
    // let seq = Sequencer::spawn(8, 8.0, Scale::new(SCALES[0]), base, conn_out);
    // seq.set_note(0, 1);
    // seq.set_note(1, 2);
    // seq.set_note(2, 2);
    // seq.set_note(3, 5);
    // seq.set_note(4, 4);
    // seq.set_note(5, 3);
    // seq.set_note(6, 5);
    // seq.set_note(7, 7);
    // seq.start();
    // seq.set_cutoff(0);
    // for i in 0..100 {
    //     seq.set_cutoff(i);
    //     sleep(Duration::from_millis(50));
    // }

    // seq.set_scale(Scale::new(SCALES[1]));
    // seq.set_base_note(38);
    // for i in 0..100 {
    //     seq.set_cutoff(99-i);
    //     sleep(Duration::from_millis(50));
    // }

    // seq.set_scale(Scale::new(SCALES[2]));
    // seq.set_base_note(48);
    // for i in 0..100 {
    //     seq.set_cutoff(i);
    //     sleep(Duration::from_millis(50));
    // }
    
    // seq.set_scale(Scale::new(SCALES[3]));
    // seq.set_base_note(36);
    // for i in 0..100 {
    //     seq.set_cutoff(99-i);
    //     sleep(Duration::from_millis(50));
    // }
    
    // seq.set_scale(Scale::new(SCALES[4]));
    // seq.set_base_note(60);
    // for i in 0..100 {
    //     seq.set_cutoff(i);
    //     sleep(Duration::from_millis(50));
    // }

    // seq.stop();

    let seq = Sequencer::spawn(4, 4.0, Scale::new(SCALES[0]), 48, conn_out);
    seq.start();
    sleep(Duration::from_millis(5000));

    println!("\nConnecting to Motion Tracking Glove...");
    let hand_original = glove::connect(GLOVE_NAME, DATA_UUID);
    let hand= hand_original.clone();

    seq.start();
    let mut controller = GestureController::new(seq);
    loop {
        controller.update(&hand.lock().unwrap());
        sleep(Duration::from_millis(50));
    }

    // Ok(())
}