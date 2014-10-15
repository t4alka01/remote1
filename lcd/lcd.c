/* LCD Simulator module
 * (c) Lauri Pirttiaho, 2014
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/spi/mcp23s08.h>
#include <linux/delay.h>
#include <linux/gpio.h>

/************************************************************
 * Module config
 */

#define LCD_BUFFER_LENGTH 80
#define MODULE_NAME "lcd"

/************************************************************
 * Module data
 */
static int gpiobase = 240;
module_param(gpiobase, int, 0444);

static char lcd_buffer[LCD_BUFFER_LENGTH] =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ9876543210zyxwvuts";

static struct lcd_size {
  int characters;
  int lines;
} lcd_size = { 16, 2 };

static int bl_color = 0;

static int output_display(char *output_buffer, const char *display_buffer)
{
  switch (lcd_size.lines) {
  case 1:
    strncpy(output_buffer, display_buffer, lcd_size.characters);
    output_buffer[lcd_size.characters] = '\n';
    return lcd_size.characters+1;
    break;
  case 2:
    strncpy(output_buffer, display_buffer, lcd_size.characters);
    output_buffer[lcd_size.characters] = '\n';
    strncpy(output_buffer+lcd_size.characters+1, display_buffer+40, lcd_size.characters);
    output_buffer[2 * lcd_size.characters + 1] = '\n';
    return 2 * lcd_size.characters + 2;
    break;
  case 4:
    strncpy(output_buffer, display_buffer, lcd_size.characters);
    output_buffer[lcd_size.characters] = '\n';
    strncpy(output_buffer+lcd_size.characters+1, display_buffer+20, lcd_size.characters);
    output_buffer[2 * lcd_size.characters + 1] = '\n';
    strncpy(output_buffer+2*lcd_size.characters+2, display_buffer+40, lcd_size.characters);
    output_buffer[3 * lcd_size.characters + 2] = '\n';
    strncpy(output_buffer+3*lcd_size.characters+3, display_buffer+60, lcd_size.characters);
    output_buffer[4 * lcd_size.characters + 3] = '\n';
    return 4 * lcd_size.characters + 4;
    break;
  default:
    break;
  }
  return 0;
}

static int size_set(const char *val, const struct kernel_param *kp)
{
  int characters = 0;
  int lines = 0;

  int n_read = sscanf(val, "%dx%d", &characters, &lines);
  if (n_read != 2) return -EINVAL;

  if (lines != 1 && lines != 2 && lines != 4) return -EINVAL;
  if (characters <= 0 || characters > 80) return -EINVAL;
  if (lines*characters > 80) return -EINVAL;

  struct lcd_size *ls = kp->arg;
  ls->characters = characters;
  ls->lines = lines;

  return 0;
}

static int size_get(char *val, const struct kernel_param *kp)
{
  struct lcd_size *ls = kp->arg;
  return sprintf(val, "%dx%d", ls->characters, ls->lines);
}

static int display_set(const char *val, const struct kernel_param *kp)
{
  // Do nothing!
  return -EPERM;
}

static int display_get(char *val, const struct kernel_param *kp)
{
  return output_display(val, (char *)kp->arg);
}

static int bl_set(const char *val, const struct kernel_param *kp);

static int bl_get(char *val, const struct kernel_param *kp)
{
  return sprintf(val, "0x%08x", bl_color);
}

static struct kernel_param_ops size_ops = {
  .set = size_set,
  .get = size_get
};

static struct kernel_param_ops display_ops = {
  .set = display_set,
  .get = display_get
};

static struct kernel_param_ops bl_ops = {
  .set = bl_set,
  .get = bl_get
};

module_param_cb(lcd_size, &size_ops, &lcd_size, 0644);
module_param_cb(display, &display_ops, lcd_buffer, 0644);
module_param_cb(bl_color, &bl_ops, &bl_color, 0644);

/************************************************************
 * LCD HW driver
 */

#define BL_RED   6
#define BL_GREEN 7
#define BL_BLUE  8

#define LCD_DB7  9
#define LCD_DB6 10
#define LCD_DB5 11
#define LCD_DB4 12
#define LCD_E   13
#define LCD_RW  14
#define LCD_RS  15

#define IN(pin) gpio_request_one(gpiobase+(pin), GPIOF_IN, #pin)
#define OUTH(pin) gpio_request_one(gpiobase+(pin), GPIOF_OUT_INIT_HIGH, #pin)
#define OUTL(pin) gpio_request_one(gpiobase+(pin), GPIOF_OUT_INIT_LOW, #pin)
#define FREE(pin) gpio_free(gpiobase+(pin))

#define SET(pin, value) gpio_set_value_cansleep(gpiobase+(pin), (value))
#define GET(pin) gpio_get_value_cansleep(gpiobase+(pin))

static int bl_set(const char *val, const struct kernel_param *kp)
{
  int color;
  sscanf(val,"0x%x", &color);

  if ((color&0xff) > 0x7f) {
    SET(BL_BLUE, 0);
  } else {
    SET(BL_BLUE, 1);
  }

  if (((color>>8)&0xff) > 0x7f) {
    SET(BL_GREEN, 0);
  } else {
    SET(BL_GREEN, 1);
  }

  if (((color>>16)&0xff) > 0x7f) {
    SET(BL_RED, 0);
  } else {
    SET(BL_RED, 1);
  }

  return 0;
}
static void write_nybble(int n)
{
  SET(LCD_DB4, (n>>0) & 1);
  SET(LCD_DB5, (n>>1) & 1);
  SET(LCD_DB6, (n>>2) & 1);
  SET(LCD_DB7, (n>>3) & 1);
  SET(LCD_E, 1);
  SET(LCD_E, 0);
}

static void write_byte(int b)
{
  write_nybble(b>>4);
  write_nybble(b>>0);
}

static void write_data(int b)
{
  SET(LCD_RS, 1);
  write_byte(b);
}

static void write_command(int b)
{
  SET(LCD_RS, 0);
  write_byte(b);
}

static void lcd_hw_init(void)
{
  OUTH(BL_RED);
  OUTH(BL_GREEN);
  OUTH(BL_BLUE);
    
  OUTL(LCD_DB7);
  OUTL(LCD_DB6);
  OUTL(LCD_DB5);
  OUTL(LCD_DB4);
  OUTL(LCD_E);
  OUTL(LCD_RW); // This is always down!
  OUTL(LCD_RS);
  
  // See fig 24 in HD44780 datasheet
  write_nybble(3);
  mdelay(4);
  write_nybble(3);
  write_nybble(3);
  write_nybble(2);

  write_command(0x28); // Funtion set: 2 lines 5x8 font
  write_command(0x0C); // Display on
  write_command(0x06); // Left to right
}

static void lcd_hw_exit(void)
{
  FREE(BL_RED);
  FREE(BL_GREEN);
  FREE(BL_BLUE);
    
  FREE(LCD_DB7);
  FREE(LCD_DB6);
  FREE(LCD_DB5);
  FREE(LCD_DB4);
  FREE(LCD_E);
  FREE(LCD_RW);
  FREE(LCD_RS);
}

static const int row_start[] = {0, 64, 20, 84};

static void lcd_write_line(int line)
{
  write_command(0x80 + row_start[line]); // Set DDRAM address

  int row_offset = line * 20;
  if (lcd_size.lines == 2) {
    row_offset = line * 40;
  }

  for (int i = 0; i < lcd_size.characters; ++i) {
    write_data(lcd_buffer[row_offset + i]);
  }
}

static void lcd_write_buffer_to_hw(void)
{
  for (int i = 0; i < lcd_size.lines; ++i) {
    lcd_write_line(i);
  }
}

/************************************************************
 * Adafruit 1110 init/exit
 */

static struct i2c_adapter* bus1;
static struct i2c_client* cli32;

static int ada_init(void)
{
  struct mcp23s08_platform_data mcp23017_pfdata = {
    .chip = {
      [0] = {
	.is_present = 1,
	.pullups = 0x001F /* pins [4:0] */
      }
    },
    .base = gpiobase
  };

  struct i2c_board_info mcp23017_info = {
    .type = "mcp23017",
    .addr = 32,
    .platform_data = &mcp23017_pfdata
  };

  bus1 = i2c_get_adapter(1);
  cli32 = i2c_new_device(bus1, &mcp23017_info); 

  if (!cli32) return -ENXIO;

  return 0;
}

static void ada_exit(void)
{
  if (cli32) {
    i2c_unregister_device(cli32);
  }
  if (bus1) {
    i2c_put_adapter(bus1);
  }
}

/************************************************************
 * Write stream parser
 */

#define NCOLS lcd_size.characters
#define NROWS lcd_size.lines

/* Parser state */

typedef struct write_stream_parser {
  int col;
  int row;
  int clear_from;
  int clear_count;
  int ansi_n;
  int ansi_m;
  const char *buffer;
  size_t len;
  int index;
  void (*state_fn) (struct write_stream_parser *);
} write_stream_parser_t;

/* Parser state function protos */
static void wsp_scroll(write_stream_parser_t *parser);
static void wsp_copy(write_stream_parser_t *parser);
static void wsp_clear(write_stream_parser_t *parser);
static void wsp_csi(write_stream_parser_t *parser);
static void wsp_ansi_n(write_stream_parser_t *parser);
static void wsp_ansi_m(write_stream_parser_t *parser);
static void wsp_ed(write_stream_parser_t *parser);
static void wsp_cup(write_stream_parser_t *parser);

/* Parser state functions */

static void wsp_copy(write_stream_parser_t *parser)
{
  if (parser->buffer[parser->index] == 0x1B /*ESC*/) {
    ++parser->index;
    parser->state_fn = wsp_csi;
    return;
  }

  // State change conditions
  if (parser->buffer[parser->index] == '\n') {
    ++parser->index;
    ++parser->row;
    parser->col = 0;
    return;
  }

  // -- scroll if on last line
  if (parser->row == NROWS) {
    parser->state_fn = wsp_scroll;
    return;
  }

  // In state process
  int lcd_index = parser->col + parser->row * 80 / NROWS;
  lcd_buffer[lcd_index] = parser->buffer[parser->index];
  ++parser->index;
  if (++parser->col == NCOLS) {
    parser->col = 0;
    ++parser->row;
  }
}

static void wsp_scroll(write_stream_parser_t *parser)
{
  switch (NROWS) {
  case 1:
    parser->clear_from = 0;
    parser->clear_count = 80;
    break;
  case 2:
    for (int i = 0; i < 40; ++i) {
      lcd_buffer[i] = lcd_buffer[i+40];
    }
    parser->clear_from = 40;
    parser->clear_count = 40;
    break;
  case 4:
    for (int i = 0; i < 60; ++i) {
      lcd_buffer[i] = lcd_buffer[i+20];
    }
    parser->clear_from = 60;
    parser->clear_count = 20;
    break;
  }
  --parser->row;
  
  parser->state_fn = wsp_clear;
}

static void wsp_clear(write_stream_parser_t *parser)
{
  while (parser->clear_count--) {
    lcd_buffer[parser->clear_from++] = ' ';
  }

  parser->state_fn = wsp_copy;
}

static void wsp_csi(write_stream_parser_t *parser)
{
  // Ignore lone ESC, assume next character is printable
  // even though in real ANSI that isn't really so
  if (parser->buffer[parser->index] != '[') {
    parser->state_fn = wsp_copy;
    return;
  }

  ++parser->index;
  
  parser->ansi_n = 0;
  if (parser->buffer[parser->index] == ';') {
    parser->ansi_n = 1;
  }
  parser->ansi_m = 0;
  parser->state_fn = wsp_ansi_n;  
}

static void wsp_ansi_n(write_stream_parser_t *parser)
{
  if ('0' <= parser->buffer[parser->index] &&
      parser->buffer[parser->index] <= '9' ) {
    parser->ansi_n *= 10;
    parser->ansi_n += parser->buffer[parser->index] - '0';
    ++parser->index;
    return;
  }

  if (parser->buffer[parser->index] == ';') {
    ++parser->index;

    if (parser->buffer[parser->index] < '0' ||
	'9' < parser->buffer[parser->index] ) {
      parser->ansi_m = 1;
    }

    parser->state_fn = wsp_ansi_m;
    return;
  }

  if (parser->buffer[parser->index] == 'J') {
    ++parser->index;
    parser->state_fn = wsp_ed;
    return;
  }

  // Others treated as normal text
  parser->state_fn = wsp_copy;
}

static void wsp_ansi_m(write_stream_parser_t *parser)
{
  if ('0' <= parser->buffer[parser->index] &&
      parser->buffer[parser->index] <= '9' ) {
    parser->ansi_m *= 10;
    parser->ansi_m += parser->buffer[parser->index] - '0';
    ++parser->index;
    return;
  }

  if (parser->buffer[parser->index] == 'H') {
    ++parser->index;
    parser->state_fn = wsp_cup;
    return;
  }

  // Others treated as normal text
  parser->state_fn = wsp_copy;
}

static void wsp_ed(write_stream_parser_t *parser)
{
  int lcd_index = parser->col + parser->row * 80 / NROWS;

  switch (parser->ansi_n) {
  case 0:
    // n == 0: clear form cursor to end
    parser->clear_from = lcd_index;
    parser->clear_count = 80 - lcd_index;
    break;
  case 1:
    // n == 1: clear from beginning to cursor
    parser->clear_from = 0;
    parser->clear_count = lcd_index + 1;
    break;
  case 2:
    // n == 2: clear entire screen
    parser->clear_from = 0;
    parser->clear_count = 80;
    break;
  }
  parser->state_fn = wsp_clear;
}

static void wsp_cup(write_stream_parser_t *parser)
{
  // go to n,m (n, m are 1 based)
  int row = parser->ansi_n - 1;
  int col = parser->ansi_m - 1;
  if (row >= NROWS) {
    row = NROWS - 1;
  }
  if (col >= NCOLS) {
    col = NCOLS - 1;
  }
  parser->row = row;
  parser->col = col;

  parser->state_fn = wsp_copy;
}

/* Parser state driver */

static void wsp_init(write_stream_parser_t *parser)
{
  parser->col = 0;
  parser->row = 0;
  parser->len = 0;
  parser->state_fn = wsp_copy;
} 

static void wsp_process_init(write_stream_parser_t *parser, const char *buffer, size_t len)
{
  parser->buffer = buffer;
  parser->len = len;
  parser->index = 0;
} 

static void wsp_process(write_stream_parser_t *parser)
{
  while (parser->index < parser->len) {
    parser->state_fn(parser);
  }
}

/************************************************************
 * Fileops
 */

typedef enum {
  DO_READ,
  READ_DONE
} read_state_e;

typedef struct {
  write_stream_parser_t parser;
  read_state_e read_state;
} file_state_t;

static int dev_open(struct inode *inode, struct file *filp)
{
  file_state_t *fs = kmalloc(sizeof(file_state_t), GFP_KERNEL);
  if (!fs) return -ENOMEM;

  filp->private_data = fs;

  fs->read_state = DO_READ;
  wsp_init(&fs->parser);

  return 0;
}

static ssize_t dev_read(
    struct file *filp, char __user *ubuff, size_t len, loff_t *offs)
{
  file_state_t *fs = filp->private_data;
  if (fs->read_state == READ_DONE) {
    fs->read_state = DO_READ;
    return 0;
  }

  int ret = 0;
  char *buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);

  if (!buffer) return -ENOMEM;

  int blen = output_display(buffer, lcd_buffer);

  if (blen < len) {
    len = blen;
  }

  int uncopied = copy_to_user(ubuff, buffer, len);

  if (uncopied) {
    ret = -EFAULT;
  } else {
    ret = len;
  }

  kfree(buffer);

  fs->read_state = READ_DONE;

  return ret;
}

static ssize_t dev_write(struct file *filp, const char __user *ubuff, size_t len, loff_t *offs)
{
  file_state_t *fs = filp->private_data;

  char *buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
  if (!buffer) return -ENOMEM;

  int ret = 0;

  if (len > PAGE_SIZE) {
    len = PAGE_SIZE;
  }

  int uncopied = copy_from_user(buffer, ubuff, len);

  if (uncopied) {
    ret = -EFAULT;
  } else {
    ret = len;
  }

  wsp_process_init(&fs->parser, buffer, len);
  wsp_process(&fs->parser);

  kfree(buffer);

  lcd_write_buffer_to_hw(); 

  return ret;
}

static int dev_release(struct inode *inode, struct file *filp)
{
  if (filp->private_data) {
    kfree(filp->private_data);
  }
  return 0;
}

static struct file_operations fileops = {
  .owner = THIS_MODULE,
  .open = dev_open,
  .read = dev_read,
  .write = dev_write,
  .release = dev_release
};


/************************************************************
 * Device data
 */

static struct class *class;
static dev_t devnum;
struct cdev cdev;
struct device *dev;

/************************************************************
 * Init and exit
 */

static int lcd_init(void)
{
  int err = 0;

  // Create device class
  class = class_create(THIS_MODULE, MODULE_NAME);

  // Allocate device number
  err = alloc_chrdev_region(&devnum, 0, 1, MODULE_NAME);
  if (err) {
    goto devnum_fail;
  }

  // Create cdev
  cdev_init(&cdev, &fileops);
  err = cdev_add(&cdev, devnum, 1);
  if (err) {
    goto dev_add_fail;
  }

  // Create device to /dev
  dev = device_create(class, NULL, devnum, NULL, MODULE_NAME);
  if (IS_ERR(dev)) {
    err = PTR_ERR(dev);
    goto dev_create_fail;
  }

  err = ada_init();
  if (err) {
    goto ada_init_fail;
  }

  lcd_hw_init();
  // All OK
  return 0;

 ada_init_fail:
  device_destroy(class, devnum);  
  
 dev_create_fail:
  cdev_del(&cdev);

 dev_add_fail:
  unregister_chrdev_region(devnum, 1);  

 devnum_fail:
  class_destroy(class);

  return err;
}

static void lcd_exit(void)
{
  lcd_hw_exit();
  ada_exit();
  device_destroy(class, devnum);
  cdev_del(&cdev);
  unregister_chrdev_region(devnum, 1);  
  class_destroy(class);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_AUTHOR("Lauri Pirttiaho <lapi@cw.fi>");
MODULE_LICENSE("Dual BSD/GPL");

