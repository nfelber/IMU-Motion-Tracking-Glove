# IMU Motion Tracking Glove

We propose a simple open source motion tracking glove prototype based on inertial measurement units (IMUs).
This project was meant to be achieved within a limited amount of time and there are therefore no plans to improve it further.
There are however a lot of potential improvement that can be made starting from our prototype, and anybody is welcome to use it for inspiration or as a base to build upon.

**Authors**: Sara Anejjar, Nathan Felber, Jad Tala, Nicolas ***REMOVED***.

![Our glove prototype](hardware/images/glove-picture1.jpg)

# Scope of the project

Many different technologies exist to perform hand and finger tracking, each with their advantages and drawbacks that we will not attempt to detail here.
We decided to explore the usage of IMUs for our prototype, as it is a technology which didn't seem to have been extensively used in motion tracking glove.
Our goal was to build a motion tracking glove from scratch without depending on proprietary VR hardware, and without depending on external computing (such as computer vision done on the user's computer).
Unfortunately, we were not able to do absolute positioning of the hand, which would have enabled our glove to be used in VR applications, but it should still be feasible with some more effort (see #Potential improvements).
Still, we were successfully able to build a prototype which collects and processes data from 6 IMUs, determines the likeliest pose and orientation of the hand from this data and sends the resulting skeleton of the hand to the computer via bluetooth.
We also wrote a driver for the device in Rust together with 3 demo applications.

# Challenges

Inertial measurement units have the advantage of providing a lot of data compared to simpler sensors (such as flex sensors) which allows for detection of a wider range of more subtle gestures.
In theory, that is.
The real challenge is that it is not straightforward to make sense of the data, and arbitrarily complex solutions can be implemented to produce better results.
For our prototype, we limited ourselves to computing the absolute orientation of each IMU, using our single 9-DoF IMU to compensate for the drift of the 5 others.
Our hand pose estimation model then only uses the orientation of the tip of each finger relative to the orientation of the back of the hand.
See #Potential improvements.

# How to replicate

Please refer to the `hardware` folder for how to build the glove itself.
All the software can be found in the `src` folder.

# Potential improvements

- Our prototype is equipped with a camera that could be used to do absolute positioning by detecting the position and distance of two colored markers, and combining this information with the orientation of the camera. Integration of the acceleration of the hand can be fused together with this data using a complementary filter to improve responsiveness and precision. Beware that the amount of image processing that can be done on the hardware we're using is very limited.
- Our pose estimation models only accounts for the orientation of the tip of each fingers. It could be improved by also using the measured dynamic acceleration to discriminate between two finger poses with similar fingertip orientations.
