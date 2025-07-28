# Brainlift

## Overview

The software stack involved in implementing this driver is the following:

- A linux application makes graphics rendering commands, openGL or otherwise.

- The linux operating system uses the Mesa3D "application level" driver to
  handle the commands. Mesa 3D is an implementation of OpenGL, mainly for the
  linux operating system.  It translates the commands into lower level commands
  that the linux kernel can understand, and may handle some of the rendering
  itself also.

- These lower level commands are passed to the hardware device for rendering. This
  is normally a physical GPU.  Linux has a number of "hardware level" drivers for
  talking to different hardware. In our case though, we have a "software defined"
  GPU device, called virtio-gpu.  Linux has a driver for this also.

- Linux talks to the "virtio-gpu" device, which is the device that QEMU exposes when
  running a virtual machine.  QEMU receives the commands (that would normally be being
  sent to the physical GPU), and performs corresponding logic to emulate what a real computer
  would do.


In parallel to this, there is an existing protocol called VirGL for accessing the GPU from the
guest linux operating system.  If you are familiar with the concept of RPC (remote procedure call),
that is essentially what VirGL is.

- On the guest side, Mesa has a special "application level" driver "VirGL", which serializes the
  GPU calls that the application made on the linux guest

- The serialized messages are mostly passed straight through, through the virtio-gpu linux device
  driver, to QEMU running on the host.

- QEMU utilizes a library called VirGLRenderer, which deserializes the commands, and executes the
  appropriate OpenGL calls, directly on the host.

- VirGLRenderer needs some hooks inside QEMU for displaying what it is rendering, onto the physical
  screen of the host.


The above process works for a linux host and guest.  Linux uses a library called EGL for handling
the very low level functions of openGL, and this is implemented within QEMU.

However, MacOS has an analogous interface, called CGL, that performs the same function.  This interface
is not implemented in QEMU, and is what prevents GPU acceleration when using a MacOS host.



## 01-mesa-virgl.md

Goes into detail about how the Mesa driver dispatches to different graphics devices in linux

