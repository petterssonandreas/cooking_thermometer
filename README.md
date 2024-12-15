# cooking_thermometer

Simple electronics and SW project for creating a digital cooking/meat thermometer.

A probe from an old broken thermometer is used. The probe appears to be an NTC with around 200k resistance at 25C. Need to do tests at different temps to get a curve for the probe.

The probe is connected to the HW via a 2.5mm "mono" jack with tip detection.

The processor used is an STM32F070F6P6 with 32 Kbytes flash and 6 Kbytes SRAM. It comes in a TSSOP-20 package, but for ease of use I have soldered it onto a DIP-20 adapter.

The product is powered via USB or via a 3.6V Li-ion battery, which is also charged when USB is connected. The battery is recycled from another old product. A small MCP73831 is used for the battery charging.

To display information a small OLED display is intended to be used.


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
