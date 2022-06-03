use std::{thread, sync::mpsc::{Receiver, Sender, self}, time::Instant, f32::consts::PI};

use cgmath::{Quaternion, Euler, Deg, InnerSpace, Rad};
use midir::MidiOutputConnection;
use mtg_drivers::hand::{HandPart, Hand};

enum SeqMsg {
    Start,
    Stop,
    Kill,
    SetNoteInScale { step: usize, value: u8 },
    SetFreq(f64),
    SetScale(Scale),
    SetBaseNote(u8),
    SetCutoff(u8),
}

pub struct Sequencer {
    notes_in_scale: Vec<u8>,
    current_step: usize,
    current_note: u8,
    freq: f64,
    scale: Scale,
    base_note: u8,
    last_step_time: Instant,
    is_running: bool,
    midi_out: MidiOutputConnection,
}

impl Sequencer {
    pub fn spawn(steps: usize, base_freq: f64, scale: Scale, base_note: u8, midi_out: MidiOutputConnection) -> SequencerHandler {
        let (tx, rx) = mpsc::channel();
        let seq = Sequencer { notes_in_scale: vec![0; steps], current_step: 0, current_note: 255,
            freq: base_freq, scale, base_note, last_step_time: Instant::now(), is_running: false, midi_out };
        thread::spawn(move || {
            seq.main_loop(rx);
        });
        SequencerHandler { tx }
    }

    fn main_loop(mut self, rx: Receiver<SeqMsg>) {
        loop {
            if self.is_running {
                if self.last_step_time.elapsed().as_secs_f64() >= self.freq.recip() {
                    self.step();
                }
            }
            
            if !match rx.try_recv() {
                Ok(msg) => self.handle_message(msg),
                Err(re) => match re {
                    mpsc::TryRecvError::Empty => true,
                    mpsc::TryRecvError::Disconnected => { println!("Channel disconnected."); false },
                },
            } { break; }
        }
        self.midi_out.close();
        println!("Connection closed");
    }

    fn step(&mut self) {
        self.current_step = (self.current_step + 1) % self.notes_in_scale.len();
        self.last_step_time = Instant::now();
        self.update_and_play_current_step_note();
    }

    fn handle_message(&mut self, msg: SeqMsg) -> bool {
        match msg {
            SeqMsg::Start => { self.is_running = true; self.step() },
            SeqMsg::Stop => { self.is_running = false; self.play_single_note(self.current_note, false) },
            SeqMsg::Kill => return false,
            SeqMsg::SetNoteInScale { step, value } => {
                self.notes_in_scale[step] = value;
                self.update_and_play_current_step_note();
            },
            SeqMsg::SetFreq(freq) => self.freq = freq,
            SeqMsg::SetScale(scale) => {
                self.scale = scale;
                self.update_and_play_current_step_note();
            },
            SeqMsg::SetBaseNote(note) => {
                self.base_note = note;
                self.update_and_play_current_step_note();
            },
            SeqMsg::SetCutoff(value) => self.set_cutoff(value),
        }
        true
    }

    fn get_current_note_from_scale(&self) -> u8 {
        self.scale.note_from_scale(self.notes_in_scale[self.current_step], self.base_note)
    }

    fn play_single_note(&mut self, note: u8, on: bool) {
        const NOTE_ON_MSG: u8 = 0x90;
        const NOTE_OFF_MSG: u8 = 0x80;
        const VELOCITY: u8 = 0x64;
        if on {
            let _ = self.midi_out.send(&[NOTE_ON_MSG, note, VELOCITY]);
        } else {
            let _ = self.midi_out.send(&[NOTE_OFF_MSG, note, VELOCITY]);
        }
    }

    fn update_and_play_current_step_note(&mut self) {
        let new_note = self.get_current_note_from_scale();
        if new_note != self.current_note {
            self.play_single_note(self.current_note, false);
            self.play_single_note(new_note, true);
            self.current_note = new_note;
        }
    }

    fn set_cutoff(&mut self, value: u8) {
        const CONTROL_CHANGE_MSG: u8 = 0xB0;
        const CUTOFF: u8 = 0x4A;
        let _ = self.midi_out.send(&[CONTROL_CHANGE_MSG, CUTOFF, value]);
    }
}

pub struct SequencerHandler {
    tx: Sender<SeqMsg>
}

impl SequencerHandler {
    pub fn start(&self) {
        self.tx.send(SeqMsg::Start).unwrap();
    }

    pub fn stop(&self) {
        self.tx.send(SeqMsg::Stop).unwrap();
    }
    pub fn set_note(&self, step: usize, value: u8) {
        self.tx.send(SeqMsg::SetNoteInScale { step, value }).unwrap();
    }

    pub fn set_freq(&self, freq: f64) {
        self.tx.send(SeqMsg::SetFreq(freq)).unwrap();
    }

    pub fn set_scale(&self, scale: Scale) {
        self.tx.send(SeqMsg::SetScale(scale)).unwrap();
    }

    pub fn set_base_note(&self, note: u8) {
        self.tx.send(SeqMsg::SetBaseNote(note)).unwrap();
    }

    pub fn set_cutoff(&self, value: u8) {
        self.tx.send(SeqMsg::SetCutoff(value)).unwrap();
    }
}

impl Drop for SequencerHandler {
    fn drop(&mut self) {
        println!("\nClosing connection");
        self.tx.send(SeqMsg::Kill).unwrap();
    }
}

pub struct Scale {
    accumulated_intervals: Vec<u8>,
    scale_len: u8,
}

impl Scale {
    pub fn new(intervals: &[u8]) -> Scale{
        let mut accumulated_intervals = vec!();
        let mut acc = 0;
        for intv in intervals {
            accumulated_intervals.push(acc);
            acc += intv;
        }
        assert!(acc == 12);
        let scale_len = u8::try_from(accumulated_intervals.len()).unwrap();
        Scale { accumulated_intervals, scale_len }
    }

    pub fn note_from_scale(&self, n: u8, base_note: u8) -> u8 {
        let full_scales = n / self.scale_len;
        return base_note + 12 * full_scales + self.accumulated_intervals[(n - full_scales * self.scale_len) as usize];
    }
}

pub struct Knob {
    value: i32,
    listener: Box<dyn Fn(u8, &SequencerHandler)>,
    start_angle: Option<f32>,
}

impl Knob {
    pub fn new(value: i32, listener: Box<dyn Fn(u8, &SequencerHandler)>) -> Knob {
        Knob { value, listener, start_angle: None }
    }

    pub fn grab(&mut self, angle: f32) {
        self.start_angle = Some(angle);
    }

    pub fn release(&mut self, angle: f32) {
        self.value = self.compute_value(angle);
        self.start_angle = None;
    }

    pub fn update_rotation(&mut self, new_angle: f32, seq_handler: &SequencerHandler) {
        (self.listener)(self.compute_value(new_angle) as u8, seq_handler);
    }

    fn compute_value(&self, new_angle: f32) -> i32 {
        self.value + if let Some(angle) = self.start_angle {
            // let opposite_angle = if angle >= PI { angle - PI} else { angle + PI };
            // let corrected_new_angle = if new_angle > opposite_angle {
            //     new_angle - 2.0*PI
            // } else {
            //     new_angle
            // };
            // let value = ((corrected_new_angle - angle) / (2.0*PI) * 256.0);
            let mut diff = new_angle - angle;
            diff += if diff < -PI { 2.0*PI } else if diff > PI { -2.0*PI } else {0.0};
            let value = diff / (2.0*PI) * 256.0;
            println!("{}", value);
            let result = value.clamp(-(self.value as f32), (255 - self.value) as f32) as i32;
            println!("Computed value {} for knob with start angle {} and current angle {} (diff: {}).", self.value + result, angle, new_angle, diff);
            result
        } else {
            0
        }
    }
}

pub struct GestureController {
    seq_handler: SequencerHandler,
    knobs: Vec<Knob>, 
    current_knob: Option<usize>,
    current_row: usize,
    rot_axis: [f32; 3],
}

impl GestureController {
    pub fn new(seq_handler: SequencerHandler) -> GestureController {
        let mut knobs = vec!();

        // Install knobs bindings
        knobs.push(Knob::new(0, Box::new(| value, seq_handler | {
            seq_handler.set_note(0, value/2);
        })));
        knobs.push(Knob::new(0, Box::new(| value, seq_handler | {
            seq_handler.set_note(1, value/2);
        })));
        knobs.push(Knob::new(0, Box::new(| value, seq_handler | {
            seq_handler.set_note(2, value/2);
        })));
        knobs.push(Knob::new(0, Box::new(| value, seq_handler | {
            seq_handler.set_note(3, value/2);
        })));
        knobs.push(Knob::new(32, Box::new(| value, seq_handler | {
            seq_handler.set_freq((((value as f64) + 16.0) / 48.0).powi(2));
        })));
        knobs.push(Knob::new(128, Box::new(| value, seq_handler | {
            seq_handler.set_cutoff(value / 2);
        })));
        knobs.push(Knob::new(48, Box::new(| value, seq_handler | {
            seq_handler.set_base_note(value / 8);
        })));
        knobs.push(Knob::new(0, Box::new(| value, seq_handler | {
            const SCALES: [&[u8]; 5] = [
                &[2, 1, 2, 1, 2, 1, 2, 1], // diminished
                &[2, 1, 4, 1, 4], // hirajoshi
                &[1, 4, 1, 4, 2], // iwato
                &[1, 3, 1, 1, 2, 3, 1], // persian
                &[2, 3, 2, 3, 2] // egyptian
            ];
            seq_handler.set_scale(Scale::new(SCALES[(value as usize) * SCALES.len() / 256]));
        })));

        GestureController { seq_handler, knobs, current_knob: None,
            current_row: 0, rot_axis: [0.0, 0.0, 0.0] }
    }

    pub fn update(&mut self, hand: &Hand) {
        const KNOBS_PER_ROW: usize = 4;

        let new_knob: Option<usize> = match hand.get_finger_touching_thumb() {
            Some(finger) => match finger {
                HandPart::Palm => None,
                HandPart::Thumb => None,
                HandPart::Index => Some(0),
                HandPart::Middle => Some(1),
                HandPart::Ring => Some(2),
                HandPart::Little => Some(3),
            },
            None => None,
        };

        match new_knob {
            Some(nk) => println!("update new_knob: {}", nk + self.current_row * KNOBS_PER_ROW),
            None => println!("update new_knob: None {:?}", hand.get_euler(HandPart::Palm)),
        };

        if match (new_knob, self.current_knob) {
            (None, None) => false,
            (None, Some(_)) => true,
            (Some(_), None) => true,
            (Some(v1), Some(v2)) => v1 != v2,
        } {
            match self.current_knob {
                Some(index) => {
                // release old knob
                let angle = self.compute_current_angle(&hand);
                self.knobs[self.current_row * KNOBS_PER_ROW + index].release(angle);
                println!("Released knob {}.", self.current_row * KNOBS_PER_ROW + index);
                },
                None => (),
            };
            match new_knob {
                Some(index) => {
                // grab new knob
                self.compute_new_axis(&hand);
                self.compute_new_row(&hand);
                let angle = self.compute_current_angle(&hand);
                self.knobs[self.current_row * KNOBS_PER_ROW + index].grab(angle);
                println!("Grabbed knob {}.", self.current_row * KNOBS_PER_ROW + index);
                },
                None => (),
            };
        } else {
            match self.current_knob {
                Some(index) => {
                // update current knob
                let angle = self.compute_current_angle(&hand);
                self.knobs[self.current_row * KNOBS_PER_ROW + index].update_rotation(angle, &self.seq_handler);
                println!("Updated knob {}.", self.current_row * KNOBS_PER_ROW + index);
                },
                None => (),
            };
        }

        self.current_knob = new_knob;
    }

    fn compute_current_angle(&self, hand: &Hand) -> f32 {
        let euler: [f32; 3] = hand.get_euler(HandPart::Palm);
        //let q: Quaternion<f32> = Quaternion::from(Euler {
            //x: Rad(euler[0]),
            //y: Rad(euler[1]),
            //z: Rad(euler[2]),
        //});
        //let ra = [q.v.x, q.v.y, q.v.y]; // rotation axis
        //let scaling = (ra[0] * self.rot_axis[0] + ra[1] * self.rot_axis[1] + ra[2] * self.rot_axis[2]) /
            //(self.rot_axis[0] * self.rot_axis[0] + self.rot_axis[1] * self.rot_axis[1] + self.rot_axis[2] * self.rot_axis[2]);
        //let p = [self.rot_axis[0] * scaling, self.rot_axis[1] * scaling, self.rot_axis[2] * scaling]; // return projection v1 on to v2  (parallel component)
        //let mut twist: Quaternion<f32> = Quaternion::new(q.s, p[0], p[1], p[2]).normalize();
        //if scaling < 0.0 { twist = -twist; }
        //return 2.0 * twist.s.acos();
        euler[1]
    }

    fn compute_new_axis(&mut self, hand: &Hand) {
        let p1 = hand.get_palm_coords()[0];
        let p2 = hand.get_thumb_coords()[0];
        // self.rot_axis = [p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]];
        self.rot_axis = [0.0, 0.0, 1.0];
    }

    fn compute_new_row(&mut self, hand: &Hand) {
        let euler: [f32; 3] = hand.get_euler(HandPart::Palm);
        self.current_row = if euler[1].abs() > 1.5 { 0 } else { 1 };
    }
}