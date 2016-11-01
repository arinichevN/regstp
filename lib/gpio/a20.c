
#define A20_PIO_BASE (0x01C20800) //PIO base address
#define A20_CCU_BASE    (0x01C20000) //CCU base address
#define MAP_SIZE (4096*2)
#define MAP_MASK (MAP_SIZE - 1)
#define BLOCK_SIZE  (4*1024)
#define A20_GPIO_NUM  64

static volatile uint32_t *gpio;

static int a20_physToGpio[A20_GPIO_NUM] = {
    -1, // 0
    -1, -1, //1, 2
    53, -1, //3, 4
    52, -1, //5, 6
    226, 228, //7, 8
    -1, 229, //9, 10
    275, 259, //11, 12
    274, -1, //13, 14
    273, 244, //15, 16
    -1, 245, //17, 18
    268, -1, //19, 20
    269, 272, //21, 22
    267, 266, //23, 24
    -1, 270, //25, 26
    257, 256, //27, 28
    35, -1, //29, 30
    277, 276, //31, 32
    45, -1, //33, 34
    39, 38, //35, 36
    37, 44, //37, 38
    -1, 40, //39, 40
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //41-> 55
    -1, -1, -1, -1, -1, -1, -1, -1 // 56-> 63
};
static uint32_t a20_gpio_data_seek [A20_GPIO_NUM];
static int a20_gpio_data_bank [A20_GPIO_NUM];
static int a20_gpio_data_index [A20_GPIO_NUM];

static uint32_t a20_gpio_data_mode_seek [A20_GPIO_NUM];
static int a20_gpio_data_mode_offset [A20_GPIO_NUM];

static int a20_pin_mask[9][32] = //[BANK]  [INDEX]
{
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PA
    { -1, -1, -1, 3, -1, 5, 6, 7, 8, -1, -1, -1, 12, 13, -1, -1, -1, -1, -1, -1, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PB
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PC
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PD
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PE
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PF
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PG
    { -1, -1, 2, -1, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PH
    {0, 1, -1, 3, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, -1, 16, 17, 18, 19, 20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, //PI
};


 void digitalWrite(int pin, int value) {
    static uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_seek[pin]);
    if (value) {
        regval |= (1 << a20_gpio_data_index[pin]);
    } else {
        regval &= ~(1 << a20_gpio_data_index[pin]);
    }
    *(gpio + a20_gpio_data_seek[pin]) = regval;
}

 int digitalRead(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_seek[pin]);
    regval = regval >> a20_gpio_data_index[pin];
    regval &= 1;
    return (int) regval;
}

 void pinLow(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_seek[pin]);
    regval &= ~(1 << a20_gpio_data_index[pin]);
    *(gpio + a20_gpio_data_seek[pin]) = regval;
}

 void pinHigh(int pin) {
    static uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_seek[pin]);
    regval |= (1 << a20_gpio_data_index[pin]);
    *(gpio + a20_gpio_data_seek[pin]) = regval;
}

 void pinModeIn(int pin) {
    uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_mode_seek[pin]);
    regval &= ~(7 << a20_gpio_data_mode_offset[pin]);
    *(gpio + a20_gpio_data_mode_seek[pin]) = regval;
}

 void pinModeOut(int pin) {
    uint32_t regval = 0;
    regval = *(gpio + a20_gpio_data_mode_seek[pin]);
    regval &= ~(7 << a20_gpio_data_mode_offset[pin]);
    regval |= (1 << a20_gpio_data_mode_offset[pin]);
    *(gpio + a20_gpio_data_mode_seek[pin]) = regval;
}

 void makeGpioDataOffset() {
    int i, pin;
    for (i = 0; i < A20_GPIO_NUM; i++) {
        pin = a20_physToGpio[i]; //WPI_MODE_PHYS
        if (-1 == pin) {
            a20_gpio_data_seek[i] = 0;
            a20_gpio_data_bank[i] = 0;
            a20_gpio_data_index[i] = 0;
            a20_gpio_data_mode_seek[i] = 0;
            a20_gpio_data_mode_offset[i] = 0;
            continue;
        }
        int bank = pin >> 5;
        int index = pin - (bank << 5);
        int mode_offset = ((index - ((index >> 3) << 3)) << 2);
        uint32_t phyaddr = A20_PIO_BASE + (bank * 36) + 0x10; // +0x10 -> data reg
        uint32_t mode_phyaddr = A20_PIO_BASE + (bank * 36) + ((index >> 3) << 2);
        if (a20_pin_mask[bank][index] != -1) {
            uint32_t mmap_base = (phyaddr & ~MAP_MASK);
            a20_gpio_data_seek[i] = ((phyaddr - mmap_base) >> 2);
            a20_gpio_data_bank[i] = bank;
            a20_gpio_data_index[i] = index;

            mmap_base = (mode_phyaddr & ~MAP_MASK);
            a20_gpio_data_mode_seek[i] = ((mode_phyaddr - mmap_base) >> 2);
            a20_gpio_data_mode_offset[i] = mode_offset;
        } else {
            a20_gpio_data_seek[i] = 0;
            a20_gpio_data_bank[i] = 0;
            a20_gpio_data_index[i] = 0;
            a20_gpio_data_mode_seek[i] = 0;
            a20_gpio_data_mode_offset[i] = 0;
        }
    }
}

int checkPin(int pin) {
    if (pin < 0 || pin >= A20_GPIO_NUM) {
        return 0;
    }
    if (a20_physToGpio[pin] == -1) {
        return 0;
    }
    return 1;
}

int gpioSetup() {
    int fd;
    if (geteuid() != 0) {
        fputs("gpioSetup: Must be root. (Did you forget sudo?)\n", stderr);
        return 0;
    }

    // Open the master /dev/memory device

    if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0) {
        fputs("gpioSetup: Unable to open /dev/mem: %s\n", stderr);
        return 0;
    }
    // GPIO:
    gpio = (volatile uint32_t *) mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, A20_CCU_BASE);
    if (gpio == MAP_FAILED) {
        fputs("gpioSetup: mmap (GPIO) failed: %s\n", stderr);
        return 0;
    }
    makeGpioDataOffset();
    return 1;
}



