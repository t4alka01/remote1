#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>


#define MODULENAME "virtualkb"

#define BUF_SIZE    80

static dev_t first;         // Global variable for the first device number
static struct cdev c_dev;   // Global variable for the character device structure
static struct class *cl;    // Global variable for the device class
static char c;

static int row_nbr = 1;
static int column_nbr = 80;

static char my_buf[ BUF_SIZE ];
static char *my_buf_ptr;

//---------------------------------------------------------------------------------

static int row_nbr_set( const char *val, const struct kernel_param *kp )
{
    long l;
    int ret;
    
    ret = kstrtol( val, 0, &l );
    if( ret < 0 || (( int )l != l ))
    {
        return ret < 0 ? ret : -EINVAL;
    }
    switch( ( int )l )
    {
        case 1:
            column_nbr = 80;
        break;
        case 2:
            column_nbr = 40;
        break;
        case 4:
            column_nbr = 20;
        break;
        default:
            l = row_nbr;
    }
    *(( int * )kp -> arg ) = l;
    
    printk( KERN_INFO MODULENAME ": row_nbr: %ld\n", l );
    
    return 0;
}

static int row_nbr_get( char *buffer, const struct kernel_param *kp )
{
    return scnprintf( buffer, PAGE_SIZE, "%d", *(( int * )kp -> arg ));
}

static struct kernel_param_ops row_nbr_pops =
{
    .get = row_nbr_get,
    .set = row_nbr_set
};

module_param_cb( row_nbr, &row_nbr_pops, &row_nbr, S_IRUGO | S_IWUSR );

static int column_nbr_set( const char *val, const struct kernel_param *kp )
{
    long l;
    int ret;
    
    ret = kstrtol( val, 0, &l );
    if( ret < 0 || (( int )l != l ))
    {
        return ret < 0 ? ret : -EINVAL;
    }
    switch( ( int )l )
    {
        case 80:
            row_nbr = 1;
        break;
        case 40:
            row_nbr = 2;
        break;
        case 20:
            row_nbr = 4;
        break;
        default:
            l = column_nbr;
    }
    *(( int * )kp -> arg ) = l;
    
    printk( KERN_INFO MODULENAME ": column_nbr: %ld\n", l );
    
    return 0;
}

static int column_nbr_get( char *buffer, const struct kernel_param *kp )
{
    return scnprintf( buffer, PAGE_SIZE, "%d", *(( int * )kp -> arg ));
}

static struct kernel_param_ops column_nbr_pops =
{
    .get = column_nbr_get,
    .set = column_nbr_set
};

module_param_cb( column_nbr, &column_nbr_pops, &column_nbr, S_IRUGO | S_IWUSR );

static int my_buf_ptr_get( char *buffer, const struct kernel_param *kp )
{
    
    return scnprintf( buffer, PAGE_SIZE, "%s", *(( char ** )kp -> arg ));
}

static struct kernel_param_ops my_buf_ptr_pops =
{
    .get = my_buf_ptr_get
};

module_param_cb( my_buf_ptr, &my_buf_ptr_pops, &my_buf_ptr, S_IRUGO );

//---------------------------------------------------------------------------------

static int my_open( struct inode *i, struct file *f )
{
    printk( KERN_INFO MODULENAME ": open()\n" );

    return 0;
}

static int my_close( struct inode *i, struct file *f )
{
    printk( KERN_INFO MODULENAME ": close()\n" );
    return 0;
}

static loff_t my_seek( struct file *f, loff_t off, int orig )
{
    loff_t new_pos;
    
    printk( KERN_INFO MODULENAME ": llseek\n" );
    
    new_pos = 0;
    switch( orig )
    {
        case 0: /* SEEK_SET: */
            new_pos = off;
        break;
        case 1: /* SEEK_CUR: */
            new_pos = f -> f_pos + off;
        break;
        case 2: /* SEEK_END: */
            new_pos = BUF_SIZE - off;
        break;
        default:
            return -EINVAL;
    }
    if( new_pos > BUF_SIZE )
    {
        new_pos = BUF_SIZE;
    }
    if( new_pos < 0 )
    {
        new_pos = 0;
        //~ return -EINVAL;
    }
    f -> f_pos = new_pos;
    printk( KERN_INFO MODULENAME ": llseek() pos=%d\n", ( int )new_pos );
    
    return new_pos;
}

static ssize_t my_read( struct file *f, char __user *buf, size_t len, loff_t *off )
{
// TODO: check if len is big enough

    int i, j;
    
    printk( KERN_INFO MODULENAME ": read() off=%d %d %d\n", ( int )*off, ( int )f -> f_pos, ( int )len );
    
    if( *off >= BUF_SIZE )
    {
        return 0; // EOF
    }
    if( *off + len > BUF_SIZE )
    {
        len = BUF_SIZE - *off;
    }
    
    j = 0;
    for( i = 0; i < len; i++ )
    {
        c = my_buf[ *off + i ];
        if( copy_to_user( buf + j, &c, 1 ) != 0 )
        {
            return -EFAULT;
        }
        j++;
        switch( column_nbr )
        {
            case 80:
                if( *off + i != 80 - 1 ) break;
            case 40:
                if( *off + i != 80 - 1 && *off + i != 40 - 1 ) break;
            case 20:
                if( *off + i != 80 - 1 && *off + i != 60 - 1 && *off + i != 40 - 1 && *off + i != 20 - 1 ) break;
                if( copy_to_user( buf + j, "\n", 1 ) != 0 )
                {
                    return -EFAULT;
                }
                j++;
                printk( KERN_INFO MODULENAME ": read() j=%d\n", j );
            break;
        }
    }
    *off += len;

//    return len;
    return j;
}

static ssize_t my_write( struct file *f, const char __user *buf, size_t len, loff_t *off )
{
    int i;

    printk( KERN_INFO MODULENAME ": write() off=%d\n", ( int )*off );

    if( *off >= BUF_SIZE )
    {
        return 0; // EOF
    }
    if( *off + len > BUF_SIZE )
    {
        len = BUF_SIZE - *off;
    }
    for( i = 0; i < len; i++ )
    {
        if( copy_from_user( &c, buf + i, 1 ) != 0 )
        {
            return -EFAULT;
        }
        my_buf[ *off + i ] = c;
    }
    *off += len;
    printk( KERN_INFO MODULENAME ": write() end off=%d\n", ( int )*off );

    return len;
}

static struct file_operations my_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write,
    .llseek = my_seek
};

static int __init mod_init( void ) /* Constructor */
{
    printk( KERN_INFO MODULENAME ": registered\n" );

    memset( my_buf, '_', BUF_SIZE );
    my_buf_ptr = my_buf;

    if( alloc_chrdev_region( &first, 0, 1, MODULENAME "_chrdevreg" ) < 0 )
    {
        return -1;
    }
    if(( cl = class_create( THIS_MODULE, MODULENAME "_class" )) == NULL )
    {
        unregister_chrdev_region( first, 1 );
        return -1;
    }
    if( device_create( cl, NULL, first, NULL, MODULENAME "_dev" ) == NULL )
    {
        class_destroy( cl );
        unregister_chrdev_region( first, 1 );
        return -1;

    }
    cdev_init( &c_dev, &my_fops );
    if( cdev_add( &c_dev, first, 1 ) == -1 )
    {
        device_destroy( cl, first );
        class_destroy( cl );
        unregister_chrdev_region( first, 1 );
        return -1;
    }
    return 0;
}

static void __exit mod_exit( void ) /* Destructor */
{
    cdev_del( &c_dev );
    device_destroy( cl, first );
    class_destroy( cl );
    unregister_chrdev_region( first, 1 );
    printk( KERN_INFO MODULENAME ": unregistered\n" );
}

module_init( mod_init );
module_exit( mod_exit );
MODULE_LICENSE( "Dual BSD/GPL" );
MODULE_AUTHOR( "Kari O" );
MODULE_DESCRIPTION( MODULENAME );
