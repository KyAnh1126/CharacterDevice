#include <linux/init.h>           
#include <linux/module.h>        
#include <linux/device.h>        
#include <linux/kernel.h>        
#include <linux/fs.h>            
#include <linux/uaccess.h>        
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#define  DEVICE_NAME "ebbchar"   
#define  CLASS_NAME  "ebb"     

MODULE_LICENSE("GPL");           
MODULE_AUTHOR("Ky Anh");    
MODULE_DESCRIPTION("A simple Linux char driver for the BBB"); 
MODULE_VERSION("0.1");    

static int    majorNumber;             
static char   message[256] = {0};         
static short  size_of_message;           
static int    numberOpens = 0;         
static struct class*  ebbcharClass  = NULL; 
static struct device* ebbcharDevice = NULL;

static DEFINE_MUTEX(ebbchar_mutex); //khai báo mutex, ngăn chặn việc nhiều tiến trình cùng truy cập vào ebbchar file

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

//đăng ký con trỏ hàm cho các hàm cần thiết để thao tác file
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

// ebbchar_init được chạy khi load device driver vào kernel, dùng để đăng ký major number, device class và device driver của thiết bị
static int __init ebbchar_init(void){
   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

   // đăng ký majorNumber
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

   ebbcharClass = class_create(CLASS_NAME);
   if (IS_ERR(ebbcharClass)){               
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);       
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");


   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){           
      class_destroy(ebbcharClass);        
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); 
   mutex_init(&ebbchar_mutex);
   return 0;
}

// cleanup function, giải phóng tài nguyên thủ công để không bị tràn bộ nhớ
static void __exit ebbchar_exit(void){
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));    
   class_unregister(ebbcharClass);                         
   class_destroy(ebbcharClass);                        
   unregister_chrdev(majorNumber, DEVICE_NAME);           
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
   while(!mutex_trylock(&ebbchar_mutex)) {
      printk(KERN_ALERT "EBBChar: Device in use by another process");
      msleep(1000);
   }

   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

// implementation của hàm read() bên user application
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   //copy_to_user được dùng để tránh page fault, thường xảy ra khi kernel module cố ghi dữ liệu vào bộ nhớ của user application
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){           
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;            
   }
}

// implementation của hàm write() bên user application
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   char* user_data = kmalloc(100, GFP_KERNEL); //cấp phát bộ nhớ cho user_data

   //copy_from_user được dùng để hạn chế page fault xảy ra khi kernel code cố đọc dữ liệu từ bộ nhớ của user application
   if (copy_from_user(user_data, buffer, len)) {
        printk(KERN_WARNING "EBBChar: Failed to receive data from the user\n");
        kfree(user_data);
        return -EFAULT; 
    }

   sprintf(message, "%s(%zu letters)", user_data, len); // thêm số ký tự vào cuối chuỗi của user
   size_of_message = strlen(message);                 
   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   kfree(user_data); //giải phóng tài nguyên bộ nhớ sau khi sử dụng
   return len;
}

// được thực thi nếu user close một file, dùng để giải phóng tài nguyên sau khi dùng
static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&ebbchar_mutex);
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}

module_init(ebbchar_init); //đăng ký hàm ebbchar_init() nếu load module lên kernel
module_exit(ebbchar_exit); //đăng ký hàm ebbchar_exit() nếu unload module khỏi kernel
