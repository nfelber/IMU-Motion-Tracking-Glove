use pyo3::{prelude::*, types::{PyList, PyBool, PyTuple}};
use mtg_drivers::{glove, hand::HandPart};
use tokio::time::{sleep, Duration};
use uuid::Uuid;
use strum::IntoEnumIterator;


// Communications specific macros
const GLOVE_NAME: &str = "MTG";
const DATA_UUID: Uuid = Uuid::from_u128(0xdc931335_7019_4096_b1e7_42a29e570f8f);

#[tokio::main]
async fn main() -> Result<(), ()>
{
    let gil = Python::acquire_gil();
    let py = gil.python();

    let hand_handle = glove::connect(GLOVE_NAME, DATA_UUID);

    let game_code = include_str!("../../flappy_bird/bird.py");
    let game = PyModule::from_code(py, game_code, "game", "game");

    loop {
            let hand = hand_handle.lock().unwrap();
            let bent_fingers = PyList::empty(py);
            HandPart::iter()
                .filter(|p| p != &HandPart::Palm)
                .for_each(|p| bent_fingers.append(PyBool::new(py, hand.get_bent_fingers().contains(&p))).unwrap());
            let args = PyTuple::new(py, &[bent_fingers]);
            game.as_ref().unwrap().getattr("receive_glove_data").unwrap().call1(args).unwrap();
            println!("{}", bent_fingers);
            //game.unwrap().getattr("receive_glove_data").unwrap().call1(bent_fingers);
            let args = PyTuple::new(py, hand.get_acceleration());
            game.as_ref().unwrap().getattr("receive_glove_acceleration").unwrap().call1(args).unwrap();
            sleep(Duration::from_millis(50)).await;
    }
}