#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mi_char_device"
#define CLASS_NAME "mi"

// Prototipos de funciones
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Juan Manuel Losada");
MODULE_DESCRIPTION("Un driver de dispositivo de caracteres simple para Linux");
MODULE_VERSION("0.1");

// Variables globales
static int majorNumber;
static char message[256] = {0};
static short size_of_message;
static int numberOpens = 0;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

// Prototipos de funciones para el driver de dispositivo de caracteres
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

// Estructura de operaciones del archivo
static struct file_operations fops =
    {
        .open = dev_open,
        .read = dev_read,
        .write = dev_write,
        .release = dev_release,
};

// Funciones de inicialización y salida del módulo
static int __init char_init(void)
{
    printk(KERN_INFO "[CharDevice]: Inicializando el Char device\n");

    // Registrar el dispositivo de caracteres
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        // Si majorNumber es menor que cero, hubo un error por lo que se debe devolver el valor de majorNumber
        printk(KERN_ALERT "[CharDevice] Falló al registrar el char device con major number\n");
        return majorNumber;
    }
    // Informar al usuario que el dispositivo se ha registrado correctamente
    printk(KERN_INFO "[CharDevice]: Registrado correctamente con major number %d\n", majorNumber);

    // Registrar la clase del dispositivo, THIS_MODULE es el módulo actual
    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass))
    {
        // Si hay un error, se debe deshacer el registro del dispositivo porque no se puede acceder a la clase
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "[CharDevice]: Falló al registrar la clase del dispositivo\n");
        return PTR_ERR(charClass);
    }
    printk(KERN_INFO "[CharDevice]: Clase del dispositivo registrada correctamente\n");

    // Registrar el dispositivo de caracteres MKDEV(majorNumber, 0) crea el dispositivo en /dev/DEVICE_NAME
    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice))
    {
        // Si hay un error, se debe deshacer el registro de la clase del dispositivo porque no se puede acceder al dispositivo
        class_destroy(charClass);
        // Deshacer el registro del dispositivo
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "[CharDevice]: Falló al crear el dispositivo\n");
        return PTR_ERR(charDevice);
    }
    printk(KERN_INFO "[CharDevice]: Dispositivo creado correctamente\n");
    return 0;
}

static void __exit char_exit(void)
{
    device_destroy(charClass, MKDEV(majorNumber, 0));
    class_unregister(charClass);
    class_destroy(charClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "[CharDevice]: Adiós del Char device!\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    numberOpens++;
    printk(KERN_INFO "[CharDevice]: El dispositivo ha sido abierto %d veces\n", numberOpens);
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int error_count = 0;
    if (*offset > 0)
    {
        // Ya se ha leído todo el mensaje, no hay más que leer
        return 0;
    }

    // Invertir el mensaje antes de enviarlo
    int i;
    char temp;
    for (i = 0; i < size_of_message / 2; i++)
    {
        temp = message[i];
        message[i] = message[size_of_message - i - 1];
        message[size_of_message - i - 1] = temp;
    }

    error_count = copy_to_user(buffer, message, size_of_message);

    if (error_count == 0)
    {
        printk(KERN_INFO "[CharDevice]: Enviado %d caracteres al usuario\n", size_of_message);
        *offset = size_of_message; // Actualizar el offset
        return size_of_message;    // Devolver la cantidad de caracteres enviados
    }
    else
    {
        printk(KERN_ERR "[CharDevice]: Error enviando %d caracteres al usuario\n", error_count);
        return -EFAULT; // Devolver un error en caso de que no se haya enviado correctamente
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    size_t to_copy = (len > sizeof(message) - 1) ? sizeof(message) - 1 : len;
    if (copy_from_user(message, buffer, to_copy) != 0)
        return -EFAULT;

    message[to_copy] = '\0'; // Asegurarse que el mensaje esté terminado correctamente
    size_of_message = strlen(message);
    printk(KERN_INFO "[CharDevice]: Recibido mensaje de %d caracteres\n", size_of_message);
    return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "[CharDevice]: Dispositivo cerrado correctamente\n");
    return 0;
}

module_init(char_init);
module_exit(char_exit);
