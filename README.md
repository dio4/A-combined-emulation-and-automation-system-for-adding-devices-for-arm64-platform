# A-combined-emulation-and-automation-system-for-adding-devices-for-arm64-platform
The system can be used for testing automation of adding devices to the system and testing system software without real hardware.
Implementation features of the combined emulation and automation system for adding devices to the arm64 system (based on RPI4):

1. The core module emulates (using the core virtualization subsystem) a high-precision BMP280 pressure and temperature sensor with a variable temperature in the project from 20 to 30 degrees. according to 
2. The reader shows the temperature in real time
3. Correct processing of termination signals
4. Automatic assembly and cleaning
5. Detailed diagnostic information output
6. Creation of "Systemd Units and Unit Files" to automate all processes of assembly, compilation, installation of modules and health checks of project components.

###############################
conclusions
###############################
Results of the implementation of the BMP280 virtual sensor:

1. Successfully implemented:
- The kernel module (virt_i2c.c) correctly emulates an I2C device and an I2C bus using the kernel virtualization subsystem
- The virtual sensor responds correctly to requests at address 0x76
- Device ID (0x58) corresponds to the real BMP280
- Emulation of temperature measurement with random fluctuations is implemented
- Correct operation with device registers

2. Functionality:
- Continuous real-time temperature update
- Realistic fluctuations in the range of 20-30°C
- Correct register read/write processing
- Correct shutdown by Ctrl+C

3. Technical aspects:
- The kernel thread is used to update the data
- The full I2C protocol is implemented
- Correct operation with the core memory
- Proper cleaning of resources when unloading the module

4. Possible improvements:
- Add pressure emulation (BMP280 - pressure and temperature sensor)
- Implement all configuration registers
- Add different operating modes (normal, forced, sleep)
- Implement calibration coefficients

The system is fully operational and can be used for testing automation of adding devices to the system and testing system software without real hardware.

PS The project implementation partially borrowed ideas from the following projects:
https://docs.kernel.org/devicetree/usage-model.html
https://www.kernel.org/doc/html/v6.13-rc7/i2c/writing-clients.html
https://github.com/mnlipp/i2c-dev-sim/blob/master/README.md
https://projectacrn.github.io/latest/developer-guides/hld/virtio-i2c.html
