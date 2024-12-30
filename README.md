# cooking_thermometer

Simple electronics and SW project for creating a digital cooking/meat thermometer.

A probe from an old broken thermometer is used. The probe appears to be an NTC with around 200k resistance at 25C. Tests done at different temps to get a curve for the probe, curve can be found in `adc.c`.

The probe is connected to the HW via a 2.5mm "mono" jack with tip detection.

The processor used is an STM32G0B0KE with 512 kB flash and 144 kB SRAM. It comes in a LQFP32 package.

The product is powered via USB or via a 3.6V Li-ion battery, which is also charged when USB is connected. The battery is recycled from another old product. A small MCP73831 is used for the battery charging.

A small OLED display is used to display temperature and other info.

An active buzzer is used for the alarm functionality.

The system runs on 3.3 V.


# Zephyr installation

### Clone this repo
```
git clone git@github.com:petterssonandreas/cooking_thermometer.git
```

### Create a virtualenv, source it
```
python3 -m venv .venv
source .venv/Scripts/activate
```

### Install west
```
pip3 install west
```

### Init west with local repo, update
```
west init -l cooking_thermometer/
west update
```

### Export CMake
```
west zephyr-export
```

### Install packages
```
west packages pip --install
```

### Install Zephyr SDK
```
west sdk install
```

# Project build and flash
```
west build -p always -b ct_g0b0ke cooking_thermometer/apps/ct/
west flash
```
