/*
 * composite.c - infrastructure for Composite USB Gadgets
 *
 * Copyright (C) 2006-2008 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* #define VERBOSE_DEBUG */

#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>

#include <linux/usb/composite.h>


/*
 * The code in this file is utility code, used to build a gadget driver
 * from one or more "function" drivers, one or more "configuration"
 * objects, and a "usb_composite_driver" by gluing them together along
 * with the relevant device-wide data.
 */

/* big enough to hold our biggest descriptor */
#define USB_BUFSIZ	1024

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : This is refered from S1.
 *                This code must be moved in mtp config function.
*/
struct os_string_descriptor_set {
	 char bLength;
	 char bDescType;
	 unsigned char qwsignature[14];
	 char bMS_VendorCode;
	 char bPad;
} __attribute__ ((packed));

typedef struct os_string_descriptor_set os_string_descriptor_set;
#endif


/*
 * Debugging macro and defines
 */
/*#define CSY_DEBUG
 *#define CSY_MORE_DEBUG
 *#define CSY_DEBUG2
 */
//#define CSY_DEBUG_ESS

#ifdef CSY_DEBUG
#  ifdef CSY_MORE_DEBUG
#    define CSY_DBG(fmt, args...) printk(KERN_INFO "usb %s:%d "fmt, __func__, __LINE__, ##args)
#  else
#    define CSY_DBG(fmt, args...) printk(KERN_INFO "usb "fmt, ##args)
#  endif
#else /* DO NOT PRINT LOG */
#  define CSY_DBG(fmt, args...) do { } while (0)
#endif /* CSY_DEBUG */

#ifdef CSY_DEBUG2
#  ifdef CSY_MORE_DEBUG
#    define CSY_DBG2(fmt, args...) printk(KERN_INFO "usb %s:%d "fmt, __func__, __LINE__, ##args)
#  else
#    define CSY_DBG2(fmt, args...) printk(KERN_INFO "usb "fmt, ##args)
#  endif
#else /* DO NOT PRINT LOG */
#  define CSY_DBG2(fmt, args...) do { } while (0)
#endif /* CSY_DEBUG2 */

#ifdef CSY_DEBUG_ESS
#  ifdef CSY_MORE_DEBUG
#    define CSY_DBG_ESS(fmt, args...) printk(KERN_INFO "usb %s:%d "fmt, __func__, __LINE__, ##args)
#  else
#    define CSY_DBG_ESS(fmt, args...) printk(KERN_INFO "usb "fmt, ##args)
#  endif
#else /* DO NOT PRINT LOG */
#  define CSY_DBG_ESS(fmt, args...) do { } while (0)
#endif /* CSY_DEBUG_ESS */

#ifdef CSY_DEBUG
#undef DBG
#  define DBG(devvalue, fmt, args...) \
	printk(KERN_INFO "usb %s:%d "fmt, __func__, __LINE__, ##args)
#endif


static struct usb_composite_driver *composite;

/* Some systems will need runtime overrides for the  product identifers
 * published in the device descriptor, either numbers or strings or both.
 * String parameters are in UTF-8 (superset of ASCII's 7 bit characters).
 */

static ushort idVendor;
module_param(idVendor, ushort, 0);
MODULE_PARM_DESC(idVendor, "USB Vendor ID");

static ushort idProduct;
module_param(idProduct, ushort, 0);
MODULE_PARM_DESC(idProduct, "USB Product ID");

static ushort bcdDevice;
module_param(bcdDevice, ushort, 0);
MODULE_PARM_DESC(bcdDevice, "USB Device version (BCD)");

static char *iManufacturer;
module_param(iManufacturer, charp, 0);
MODULE_PARM_DESC(iManufacturer, "USB Manufacturer string");

static char *iProduct;
module_param(iProduct, charp, 0);
MODULE_PARM_DESC(iProduct, "USB Product string");

static char *iSerialNumber;
module_param(iSerialNumber, charp, 0);
MODULE_PARM_DESC(iSerialNumber, "SerialNumber string");

/*-------------------------------------------------------------------------*/

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_function *f = dev_get_drvdata(dev);
	CSY_DBG2("\n");
	return sprintf(buf, "%d\n", !f->disabled);
}

static ssize_t enable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_function *f = dev_get_drvdata(dev);
	struct usb_composite_driver	*driver = f->config->cdev->driver;
	int value;
	CSY_DBG2("\n");

	sscanf(buf, "%d", &value);
	if (driver->enable_function)
		driver->enable_function(f, value);
	else
		usb_function_set_enabled(f, value);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);

void usb_function_set_enabled(struct usb_function *f, int enabled)
{
	f->disabled = !enabled;
	CSY_DBG2("name=%s, enabled=%d\n", f->name, enabled);
	kobject_uevent(&f->dev->kobj, KOBJ_CHANGE);
}


void usb_composite_force_reset(struct usb_composite_dev *cdev)
{
	unsigned long			flags;

	spin_lock_irqsave(&cdev->lock, flags);
	/* force reenumeration */
	if (cdev && cdev->gadget &&
			cdev->gadget->speed != USB_SPEED_UNKNOWN) {
		/* avoid sending a disconnect switch event until after we disconnect */
		cdev->mute_switch = 1;
		spin_unlock_irqrestore(&cdev->lock, flags);
		CSY_DBG_ESS("disconnect usb\n");
		usb_gadget_disconnect(cdev->gadget);
		msleep(10);
		CSY_DBG_ESS("connect usb again\n");
		usb_gadget_connect(cdev->gadget);
	} else {
		CSY_DBG_ESS("skip reset\n");
		spin_unlock_irqrestore(&cdev->lock, flags);
	}
}

/**
 * usb_add_function() - add a function to a configuration
 * @config: the configuration
 * @function: the function being added
 * Context: single threaded during gadget setup
 *
 * After initialization, each configuration must have one or more
 * functions added to it.  Adding a function involves calling its @bind()
 * method to allocate resources such as interface and string identifiers
 * and endpoints.
 *
 * This function returns the value of the function's bind(), which is
 * zero for success else a negative errno value.
 */
int usb_add_function(struct usb_configuration *config,
		struct usb_function *function)
{
	struct usb_composite_dev	*cdev = config->cdev;
	int	value = -EINVAL;
	int index;

	DBG(cdev, "adding '%s'/%p to config '%s'/%p\n",
			function->name, function,
			config->label, config);

	if (!function->set_alt || !function->disable)
		goto done;

	index = atomic_inc_return(&cdev->driver->function_count);
	function->dev = device_create(cdev->driver->class, NULL,
		MKDEV(0, index), NULL, function->name);
	if (IS_ERR(function->dev))
		return PTR_ERR(function->dev);

	value = device_create_file(function->dev, &dev_attr_enable);
	if (value < 0) {
		device_destroy(cdev->driver->class, MKDEV(0, index));
		return value;
	}
	dev_set_drvdata(function->dev, function);

	function->config = config;
	list_add_tail(&function->list, &config->functions);

	/* REVISIT *require* function->bind? */
	if (function->bind) {
		value = function->bind(config, function);
		if (value < 0) {
			list_del(&function->list);
			function->config = NULL;
		}
	} else
		value = 0;

	/* We allow configurations that don't work at both speeds.
	 * If we run into a lowspeed Linux system, treat it the same
	 * as full speed ... it's the function drivers that will need
	 * to avoid bulk and ISO transfers.
	 */
	if (!config->fullspeed && function->descriptors)
		config->fullspeed = true;
	if (!config->highspeed && function->hs_descriptors)
		config->highspeed = true;

done:
	if (value)
		DBG(cdev, "adding '%s'/%p --> %d\n",
				function->name, function, value);
	return value;
}

/**
 * usb_function_deactivate - prevent function and gadget enumeration
 * @function: the function that isn't yet ready to respond
 *
 * Blocks response of the gadget driver to host enumeration by
 * preventing the data line pullup from being activated.  This is
 * normally called during @bind() processing to change from the
 * initial "ready to respond" state, or when a required resource
 * becomes available.
 *
 * For example, drivers that serve as a passthrough to a userspace
 * daemon can block enumeration unless that daemon (such as an OBEX,
 * MTP, or print server) is ready to handle host requests.
 *
 * Not all systems support software control of their USB peripheral
 * data pullups.
 *
 * Returns zero on success, else negative errno.
 */
int usb_function_deactivate(struct usb_function *function)
{
	struct usb_composite_dev	*cdev = function->config->cdev;
	unsigned long			flags;
	int				status = 0;

	CSY_DBG2("\n");
	spin_lock_irqsave(&cdev->lock, flags);

	if (cdev->deactivations == 0)
		status = usb_gadget_disconnect(cdev->gadget);
	if (status == 0)
		cdev->deactivations++;

	spin_unlock_irqrestore(&cdev->lock, flags);
	return status;
}

/**
 * usb_function_activate - allow function and gadget enumeration
 * @function: function on which usb_function_activate() was called
 *
 * Reverses effect of usb_function_deactivate().  If no more functions
 * are delaying their activation, the gadget driver will respond to
 * host enumeration procedures.
 *
 * Returns zero on success, else negative errno.
 */
int usb_function_activate(struct usb_function *function)
{
	struct usb_composite_dev	*cdev = function->config->cdev;
	int				status = 0;

	CSY_DBG2("\n");
	spin_lock(&cdev->lock);

	if (WARN_ON(cdev->deactivations == 0))
		status = -EINVAL;
	else {
		cdev->deactivations--;
		if (cdev->deactivations == 0)
			status = usb_gadget_connect(cdev->gadget);
	}

	spin_unlock(&cdev->lock);
	return status;
}

/**
 * usb_interface_id() - allocate an unused interface ID
 * @config: configuration associated with the interface
 * @function: function handling the interface
 * Context: single threaded during gadget setup
 *
 * usb_interface_id() is called from usb_function.bind() callbacks to
 * allocate new interface IDs.  The function driver will then store that
 * ID in interface, association, CDC union, and other descriptors.  It
 * will also handle any control requests targetted at that interface,
 * particularly changing its altsetting via set_alt().  There may
 * also be class-specific or vendor-specific requests to handle.
 *
 * All interface identifier should be allocated using this routine, to
 * ensure that for example different functions don't wrongly assign
 * different meanings to the same identifier.  Note that since interface
 * identifers are configuration-specific, functions used in more than
 * one configuration (or more than once in a given configuration) need
 * multiple versions of the relevant descriptors.
 *
 * Returns the interface ID which was allocated; or -ENODEV if no
 * more interface IDs can be allocated.
 */
int usb_interface_id(struct usb_configuration *config,
		struct usb_function *function)
{
	unsigned id = config->next_interface_id;

	CSY_DBG2("\n");
	if (id < MAX_CONFIG_INTERFACES) {
		config->interface[id] = function;
		config->next_interface_id = id + 1;
		return id;
	}
	return -ENODEV;
}

static int config_buf(struct usb_configuration *config,
		enum usb_device_speed speed, void *buf, u8 type)
{
	struct usb_config_descriptor	*c = buf;
	struct usb_interface_descriptor *intf;
	void				*next = buf + USB_DT_CONFIG_SIZE;
	int				len = USB_BUFSIZ - USB_DT_CONFIG_SIZE;
	struct usb_function		*f;
	int				status;
	int				interfaceCount = 0;
	u8 *dest;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : set interface number dynamically based on product function sequence. */
	int i;
	char **product_functions = config->cdev->products->functions;
#endif
	/* write the config descriptor */
	c = buf;
	c->bLength = USB_DT_CONFIG_SIZE;
	c->bDescriptorType = type;
	/* wTotalLength and bNumInterfaces are written later */
	c->bConfigurationValue = config->bConfigurationValue;
	c->iConfiguration = config->iConfiguration;
	c->bmAttributes = USB_CONFIG_ATT_ONE | config->bmAttributes;
	c->bMaxPower = config->bMaxPower ? : (CONFIG_USB_GADGET_VBUS_DRAW / 2);

	/* There may be e.g. OTG descriptors */
	if (config->descriptors) {
		status = usb_descriptor_fillbuf(next, len,
				config->descriptors);
		if (status < 0)
			return status;
		len -= status;
		next += status;
	}
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : set interface number dynamically based on product function sequence. */
	CSY_DBG("config_buf config->cdev=0x%p\n", config->cdev);
	for(i=0; i < config->cdev->product_num; i++)
	{
		char *product_function   = *product_functions++;
#endif
	/* add each function's descriptors */
	list_for_each_entry(f, &config->functions, list) {
		struct usb_descriptor_header **descriptors;
		struct usb_descriptor_header *descriptor;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : set interface number dynamically based on product function sequence. */
			CSY_DBG("i=%d config-cdev->product_num=%d, p_func=%s, c_func=%s\n",
			i, config->cdev->product_num, product_function,  f->name);
			if(!strcmp(f->name, product_function))
			{
				CSY_DBG("set interface number (%s)\n", f->name);
#endif
		if (speed == USB_SPEED_HIGH)
			descriptors = f->hs_descriptors;
		else
			descriptors = f->descriptors;
				if (f->disabled || !descriptors || descriptors[0] == NULL) {
					CSY_DBG("config_buf skip %s function disabled\n",f->name);
			continue;
				}
		status = usb_descriptor_fillbuf(next, len,
			(const struct usb_descriptor_header **) descriptors);
		if (status < 0)
			return status;

		/* set interface numbers dynamically */
		dest = next;
		while ((descriptor = *descriptors++) != NULL) {
			intf = (struct usb_interface_descriptor *)dest;
			if (intf->bDescriptorType == USB_DT_INTERFACE) {
				/* don't increment bInterfaceNumber for alternate settings */

				if (intf->bAlternateSetting == 0)
					intf->bInterfaceNumber = interfaceCount++;
				else
					intf->bInterfaceNumber = interfaceCount - 1;
						CSY_DBG("config_buf %s function (interfaceCount=%d)\n",f->name, intf->bInterfaceNumber);
			}
			dest += intf->bLength;
		}

		len -= status;
		next += status;
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : set interface number dynamically based on product function sequence. */
			}
#endif
	}
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : set interface number dynamically based on product function sequence. */
	}
#endif
	len = next - buf;
	c->wTotalLength = cpu_to_le16(len);
	c->bNumInterfaces = interfaceCount;
	return len;
}

static int config_desc(struct usb_composite_dev *cdev, unsigned w_value)
{
	struct usb_gadget		*gadget = cdev->gadget;
	struct usb_configuration	*c;
	u8				type = w_value >> 8;
	enum usb_device_speed		speed = USB_SPEED_UNKNOWN;

	CSY_DBG2("\n");
	if (gadget_is_dualspeed(gadget)) {
		int			hs = 0;

		if (gadget->speed == USB_SPEED_HIGH)
			hs = 1;
		if (type == USB_DT_OTHER_SPEED_CONFIG)
			hs = !hs;
		if (hs)
			speed = USB_SPEED_HIGH;

	}

	/* This is a lookup by config *INDEX* */
	w_value &= 0xff;
	list_for_each_entry(c, &cdev->configs, list) {
		/* ignore configs that won't work at this speed */
		if (speed == USB_SPEED_HIGH) {
			if (!c->highspeed)
				continue;
		} else {
			if (!c->fullspeed)
				continue;
		}
		if (w_value == 0) {
			CSY_DBG("c=0x%p, cdev=0x%p, cdev->req=0x%p, cdev->req->buf=0x%p\n", c, cdev, cdev->req, cdev->req->buf);
			return config_buf(c, speed, cdev->req->buf, type);
		}
		w_value--;
	}
	return -EINVAL;
}

static int count_configs(struct usb_composite_dev *cdev, unsigned type)
{
	struct usb_gadget		*gadget = cdev->gadget;
	struct usb_configuration	*c;
	unsigned			count = 0;
	int				hs = 0;

	CSY_DBG2("\n");
	if (gadget_is_dualspeed(gadget)) {
		if (gadget->speed == USB_SPEED_HIGH)
			hs = 1;
		if (type == USB_DT_DEVICE_QUALIFIER)
			hs = !hs;
	}
	list_for_each_entry(c, &cdev->configs, list) {
		/* ignore configs that won't work at this speed */
		if (hs) {
			if (!c->highspeed)
				continue;
		} else {
			if (!c->fullspeed)
				continue;
		}
		count++;
	}
	return count;
}

static void device_qual(struct usb_composite_dev *cdev)
{
	struct usb_qualifier_descriptor	*qual = cdev->req->buf;

	CSY_DBG2("\n");
	qual->bLength = sizeof(*qual);
	qual->bDescriptorType = USB_DT_DEVICE_QUALIFIER;
	/* POLICY: same bcdUSB and device type info at both speeds */
	qual->bcdUSB = cdev->desc.bcdUSB;
	qual->bDeviceClass = cdev->desc.bDeviceClass;
	qual->bDeviceSubClass = cdev->desc.bDeviceSubClass;
	qual->bDeviceProtocol = cdev->desc.bDeviceProtocol;
	/* ASSUME same EP0 fifo size at both speeds */
	qual->bMaxPacketSize0 = cdev->desc.bMaxPacketSize0;
	qual->bNumConfigurations = count_configs(cdev, USB_DT_DEVICE_QUALIFIER);
	qual->bRESERVED = 0;
}

/*-------------------------------------------------------------------------*/

static void reset_config(struct usb_composite_dev *cdev)
{
	struct usb_function		*f;

	DBG(cdev, "reset config\n");

	list_for_each_entry(f, &cdev->config->functions, list) {
		if (f->disable)
			f->disable(f);

		bitmap_zero(f->endpoints, 32);
	}
	cdev->config = NULL;
}

static int set_config(struct usb_composite_dev *cdev,
		const struct usb_ctrlrequest *ctrl, unsigned number)
{
	struct usb_gadget	*gadget = cdev->gadget;
	struct usb_configuration *c = NULL;
	int			result = -EINVAL;
	unsigned		power = gadget_is_otg(gadget) ? 8 : 100;
	int			tmp;

	CSY_DBG2("\n");
	if (cdev->config)
		reset_config(cdev);

	if (number) {
		list_for_each_entry(c, &cdev->configs, list) {
			if (c->bConfigurationValue == number) {
				result = 0;
				break;
			}
		}
		if (result < 0)
			goto done;
	} else
		result = 0;

	INFO(cdev, "%s speed config #%d: %s\n",
		({ char *speed;
		switch (gadget->speed) {
		case USB_SPEED_LOW:	speed = "low"; break;
		case USB_SPEED_FULL:	speed = "full"; break;
		case USB_SPEED_HIGH:	speed = "high"; break;
		default:		speed = "?"; break;
		} ; speed; }), number, c ? c->label : "unconfigured");

	if (!c)
		goto done;

	cdev->config = c;

	/* Initialize all interfaces by setting them to altsetting zero. */
	for (tmp = 0; tmp < MAX_CONFIG_INTERFACES; tmp++) {
		struct usb_function	*f = c->interface[tmp];
		struct usb_descriptor_header **descriptors;

		if (!f)
			break;
		if (f->disabled)
			continue;

		/*
		 * Record which endpoints are used by the function. This is used
		 * to dispatch control requests targeted at that endpoint to the
		 * function's setup callback instead of the current
		 * configuration's setup callback.
		 */
		if (gadget->speed == USB_SPEED_HIGH)
			descriptors = f->hs_descriptors;
		else
			descriptors = f->descriptors;

		for (; *descriptors; ++descriptors) {
			struct usb_endpoint_descriptor *ep;
			int addr;

			if ((*descriptors)->bDescriptorType != USB_DT_ENDPOINT)
				continue;

			ep = (struct usb_endpoint_descriptor *)*descriptors;
			addr = ((ep->bEndpointAddress & 0x80) >> 3)
			     |  (ep->bEndpointAddress & 0x0f);
			set_bit(addr, f->endpoints);
		}

		result = f->set_alt(f, tmp, 0);
		if (result < 0) {
			DBG(cdev, "interface %d (%s/%p) alt 0 --> %d\n",
					tmp, f->name, f, result);

			reset_config(cdev);
			goto done;
		}
	}

	/* when we return, be sure our power usage is valid */
	power = c->bMaxPower ? (2 * c->bMaxPower) : CONFIG_USB_GADGET_VBUS_DRAW;
done:
	usb_gadget_vbus_draw(gadget, power);

	schedule_work(&cdev->switch_work);
	return result;
}

/**
 * usb_add_config() - add a configuration to a device.
 * @cdev: wraps the USB gadget
 * @config: the configuration, with bConfigurationValue assigned
 * Context: single threaded during gadget setup
 *
 * One of the main tasks of a composite driver's bind() routine is to
 * add each of the configurations it supports, using this routine.
 *
 * This function returns the value of the configuration's bind(), which
 * is zero for success else a negative errno value.  Binding configurations
 * assigns global resources including string IDs, and per-configuration
 * resources such as interface IDs and endpoints.
 */
int usb_add_config(struct usb_composite_dev *cdev,
		struct usb_configuration *config)
{
	int				status = -EINVAL;
	struct usb_configuration	*c;

	DBG(cdev, "adding config #%u '%s'/%p\n",
			config->bConfigurationValue,
			config->label, config);

	if (!config->bConfigurationValue || !config->bind)
		goto done;

	/* Prevent duplicate configuration identifiers */
	list_for_each_entry(c, &cdev->configs, list) {
		if (c->bConfigurationValue == config->bConfigurationValue) {
			status = -EBUSY;
			goto done;
		}
	}

	config->cdev = cdev;
	list_add_tail(&config->list, &cdev->configs);

	INIT_LIST_HEAD(&config->functions);
	config->next_interface_id = 0;

	status = config->bind(config);
	DBG(cdev, "config->bind(%p) status=%d\n",config, status);

	if (status < 0) {
		list_del(&config->list);
		config->cdev = NULL;
	} else {
		unsigned	i;

		DBG(cdev, "cfg %d/%p speeds:%s%s\n",
			config->bConfigurationValue, config,
			config->highspeed ? " high" : "",
			config->fullspeed
				? (gadget_is_dualspeed(cdev->gadget)
					? " full"
					: " full/low")
				: "");

		for (i = 0; i < MAX_CONFIG_INTERFACES; i++) {
			struct usb_function	*f = config->interface[i];

			if (!f)
				continue;
			DBG(cdev, "  interface %d = %s/%p\n",
				i, f->name, f);
		}
	}

	/* set_alt(), or next config->bind(), sets up
	 * ep->driver_data as needed.
	 */
	usb_ep_autoconfig_reset(cdev->gadget);

done:
	if (status)
		DBG(cdev, "added config '%s'/%u --> %d\n", config->label,
				config->bConfigurationValue, status);
	return status;
}

/*-------------------------------------------------------------------------*/

/* We support strings in multiple languages ... string descriptor zero
 * says which languages are supported.  The typical case will be that
 * only one language (probably English) is used, with I18N handled on
 * the host side.
 */

static void collect_langs(struct usb_gadget_strings **sp, __le16 *buf)
{
	const struct usb_gadget_strings	*s;
	u16				language;
	__le16				*tmp;

	CSY_DBG2("\n");
	while (*sp) {
		s = *sp;
		language = cpu_to_le16(s->language);
		for (tmp = buf; *tmp && tmp < &buf[126]; tmp++) {
			if (*tmp == language)
				goto repeat;
		}
		*tmp++ = language;
repeat:
		sp++;
	}
}

static int lookup_string(
	struct usb_gadget_strings	**sp,
	void				*buf,
	u16				language,
	int				id
)
{
	struct usb_gadget_strings	*s;
	int				value;

	while (*sp) {
		s = *sp++;
		if (s->language != language)
			continue;
		value = usb_gadget_get_string(s, id, buf);
		if (value > 0)
			return value;
	}
	return -EINVAL;
}

static int get_string(struct usb_composite_dev *cdev,
		void *buf, u16 language, int id)
{
	struct usb_configuration	*c;
	struct usb_function		*f;
	int				len;

	CSY_DBG2("\n");
	/* Yes, not only is USB's I18N support probably more than most
	 * folk will ever care about ... also, it's all supported here.
	 * (Except for UTF8 support for Unicode's "Astral Planes".)
	 */

	/* 0 == report all available language codes */
	if (id == 0) {
		struct usb_string_descriptor	*s = buf;
		struct usb_gadget_strings	**sp;

		memset(s, 0, 256);
		s->bDescriptorType = USB_DT_STRING;

		sp = composite->strings;
		if (sp)
			collect_langs(sp, s->wData);

		list_for_each_entry(c, &cdev->configs, list) {
			sp = c->strings;
			if (sp)
				collect_langs(sp, s->wData);

			list_for_each_entry(f, &c->functions, list) {
				sp = f->strings;
				if (sp)
					collect_langs(sp, s->wData);
			}
		}

		for (len = 0; len <= 126 && s->wData[len]; len++)
			continue;
		if (!len)
			return -EINVAL;

		s->bLength = 2 * (len + 1);
		return s->bLength;
	}
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : This is refered from S1
 *		  This code must be moved in mtp config function
 */
	else if (id == 0xEE) {
		char **product_functions = cdev->products->functions;
		int has_mtp = 0;
		int i;

		CSY_DBG("mtp cmd id=0x%x)\n", id);
		for(i=0; i < cdev->product_num; i++) {
			char *product_function   = *product_functions++;
			if(!strcmp("mtp", product_function)) {
				CSY_DBG("this product have mtp function (id=0x%x)\n", id);
				has_mtp = 1;
			}
		}

		if(has_mtp == 1) {
			os_string_descriptor_set output;
			struct usb_string_descriptor *os_desc = buf;

			printk("this product have mtp function line = %d\n", __LINE__);
			memset(os_desc, 0x00, 256);
			os_desc->bDescriptorType = USB_DT_STRING;

			output.qwsignature[0] = 0x4D;
			output.qwsignature[1] = 0x00;
			output.qwsignature[2] = 0x53;
			output.qwsignature[3] = 0x00;
			output.qwsignature[4] = 0x46;
			output.qwsignature[5] = 0x00;
			output.qwsignature[6] = 0x54;
			output.qwsignature[7] = 0x00;
			output.qwsignature[8] = 0x31;
			output.qwsignature[9] = 0x00;
			output.qwsignature[10] = 0x30;
			output.qwsignature[11] = 0x00;
			output.qwsignature[12] = 0x30;
			output.qwsignature[13] = 0x00;

			output.bMS_VendorCode = 0x54;
			output.bPad = 0x00;

			memcpy(os_desc->wData, &output.qwsignature, sizeof(os_string_descriptor_set));
			os_desc->bLength = 0x12;
			return os_desc->bLength;
		}
	}
#endif

	/* Otherwise, look up and return a specified string.  String IDs
	 * are device-scoped, so we look up each string table we're told
	 * about.  These lookups are infrequent; simpler-is-better here.
	 */
	if (composite->strings) {
		len = lookup_string(composite->strings, buf, language, id);
		if (len > 0)
			return len;
	}
	list_for_each_entry(c, &cdev->configs, list) {
		if (c->strings) {
			len = lookup_string(c->strings, buf, language, id);
			if (len > 0)
				return len;
		}
		list_for_each_entry(f, &c->functions, list) {
			if (!f->strings)
				continue;
			len = lookup_string(f->strings, buf, language, id);
			if (len > 0)
				return len;
		}
	}
	return -EINVAL;
}

/**
 * usb_string_id() - allocate an unused string ID
 * @cdev: the device whose string descriptor IDs are being allocated
 * Context: single threaded during gadget setup
 *
 * @usb_string_id() is called from bind() callbacks to allocate
 * string IDs.  Drivers for functions, configurations, or gadgets will
 * then store that ID in the appropriate descriptors and string table.
 *
 * All string identifier should be allocated using this routine, to
 * ensure that for example different functions don't wrongly assign
 * different meanings to the same identifier.
 */
int usb_string_id(struct usb_composite_dev *cdev)
{
	if (cdev->next_string_id < 254) {
		/* string id 0 is reserved */
		cdev->next_string_id++;
		return cdev->next_string_id;
	}
	return -ENODEV;
}

/*-------------------------------------------------------------------------*/

static void composite_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	CSY_DBG("composite_setup_complete req->length=0x%x\n", req->length);
	if (req->status || req->actual != req->length)
		DBG((struct usb_composite_dev *) ep->driver_data,
				"setup complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
}

/*
 * The setup() callback implements all the ep0 functionality that's
 * not handled lower down, in hardware or the hardware driver(like
 * device and endpoint feature flags, and their status).  It's all
 * housekeeping for the gadget function we're implementing.  Most of
 * the work is in config and function specific setup.
 */
static int
composite_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_request		*req = cdev->req;
	int				value = -EOPNOTSUPP;
	u16				w_index = le16_to_cpu(ctrl->wIndex);
	u8				intf = w_index & 0xFF;
	u16				w_value = le16_to_cpu(ctrl->wValue);
	u16				w_length = le16_to_cpu(ctrl->wLength);
	struct usb_function		*f = NULL;
	u8				endp;

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	int i;
/* soonyong.cho : Added handler to respond to host about MS OS Descriptors.
 * 		  Below compatible ID is for MTP.
 *		  So, If you set composite included MTP, you have to respond to host about 0x54 request.
 *            	  Below handler must be moved in mtp function.
 */
	char ms_descriptor[38] = { 0x00, 0x00, 0x00, 0x01, 0x04, 0x00,0x01,0x00,0x00,0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x01, 
	 0x4D, 0x54, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	 
#endif
	/* partial re-init of the response message; the function or the
	 * gadget might need to intercept e.g. a control-OUT completion
	 * when we delegate to it.
	 */
	req->zero = 0;
	req->complete = composite_setup_complete;
	req->length = USB_BUFSIZ;
	CSY_DBG("++setup req->length=0x%x\n", req->length);
	gadget->ep0->driver_data = cdev;

	switch (ctrl->bRequest) {
#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
	/* soonyong.cho : Added handler to respond to host about MS OS Descriptors.
	 * 		  Below handler is requirement if you use MTP.
	 *		  So, If you set composite included MTP, you have to respond to host about 0x54 request
	 *            	  Below handler must be moved in mtp function.
	 */
	 case 0x54:
	 	if(cdev->products) {
			for(i = 0; i < cdev->product_num; i++) {
				if (!strcmp(cdev->products->functions[i], "mtp")) {
					struct usb_string_descriptor *os_func_desc = req->buf;
					CSY_DBG_ESS("0x54 request, mode=mtp, p_num=%d, f_index=%d\n",cdev->product_num,i);
					os_func_desc->bDescriptorType = 0x00;
					//memcpy(os_func_desc->wData, &ms_descriptor, sizeof(ms_descriptor));
					memcpy(os_func_desc->wData, &ms_descriptor, 40);
					//value = sizeof(ms_descriptor);
					os_func_desc->bLength = value = 0x28;
			               	if (value >= 0) {
			                    value = min(w_length, (u16) value);
					}
					break;
				}
			}
		}
		break;

#endif
	/* we handle all standard USB descriptors */
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {

		case USB_DT_DEVICE:
			CSY_DBG_ESS("GET_DES-DEV\n");
			cdev->desc.bNumConfigurations =
				count_configs(cdev, USB_DT_DEVICE);
			value = min(w_length, (u16) sizeof cdev->desc);
			memcpy(req->buf, &cdev->desc, value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			CSY_DBG("GET_DESCRIPTOR-DT_DEVICE_QUALIFIER\n");
			if (!gadget_is_dualspeed(gadget))
				break;
			device_qual(cdev);
			value = min_t(int, w_length,
				sizeof(struct usb_qualifier_descriptor));
			break;
		case USB_DT_OTHER_SPEED_CONFIG:
			CSY_DBG("GET_DESCRIPTOR-DT_OTHER_SPEED_CONFIG\n");
			if (!gadget_is_dualspeed(gadget))
				break;
			/* FALLTHROUGH */
		case USB_DT_CONFIG:
			CSY_DBG_ESS("GET_DES-CON\n");
			value = config_desc(cdev, w_value);
			if (value >= 0)
				value = min(w_length, (u16) value);
			break;
		case USB_DT_STRING:
			CSY_DBG2("GET_DESCRIPTOR-DT_STRING\n");
			value = get_string(cdev, req->buf,
					w_index, w_value & 0xff);

			/* Allow functions to handle USB_DT_STRING.
			 * This is required for MTP.
			 */
			if (value < 0) {
				struct usb_configuration        *cfg;
				list_for_each_entry(cfg, &cdev->configs, list) {
					if (cfg && cfg->setup) {
						CSY_DBG("DT-STRING value=%d, N:cfg->setup\n",value);
						value = cfg->setup(cfg, ctrl);
						if (value >= 0)
							break;
					}
				}
			}

			if (value >= 0)
				value = min(w_length, (u16) value);
			break;
		}
		break;

	/* any number of configs can work */
	case USB_REQ_SET_CONFIGURATION:
		CSY_DBG_ESS("SET_CON w_v=%d\n", w_value);
		if (ctrl->bRequestType != 0)
			goto unknown;
		if (gadget_is_otg(gadget)) {
			if (gadget->a_hnp_support)
				DBG(cdev, "HNP available\n");
			else if (gadget->a_alt_hnp_support)
				DBG(cdev, "HNP on another port\n");
			else
				VDBG(cdev, "HNP inactive\n");
		}
		spin_lock(&cdev->lock);
		value = set_config(cdev, ctrl, w_value);
		spin_unlock(&cdev->lock);
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		if (cdev->config) {
			*(u8 *)req->buf = cdev->config->bConfigurationValue;
			value = min(w_length, (u16) 1);
		} else
			*(u8 *)req->buf = 0;
		CSY_DBG("GET_CONFIGURATION value=%d\n", value);
		break;

	/* function drivers must handle get/set altsetting; if there's
	 * no get() method, we know only altsetting zero works.
	 */
	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			goto unknown;
		if (!cdev->config || w_index >= MAX_CONFIG_INTERFACES)
			break;
		f = cdev->config->interface[intf];
		if (!f)
			break;
		if (w_value && !f->set_alt)
			break;
		CSY_DBG("Before SET_INTERFACE w_index=%d w_value=%d f->name=%s\n", w_index, w_value,f->name);
		value = f->set_alt(f, w_index, w_value);
		CSY_DBG("After SET_INTERFACE w_index=%d w_value=%d f->name=%s\n", w_index, w_value,f->name);
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
			goto unknown;
		if (!cdev->config || w_index >= MAX_CONFIG_INTERFACES)
			break;
		f = cdev->config->interface[intf];
		if (!f)
			break;
		/* lots of interfaces only need altsetting zero... */
		value = f->get_alt ? f->get_alt(f, w_index) : 0;
		if (value < 0)
			break;
		*((u8 *)req->buf) = value;
		CSY_DBG("GET_INTERFACE value=%d, f->name=%s\n", value, f->name);
		value = min(w_length, (u16) 1);
		break;
	default:
unknown:
#ifdef CSY_DEBUG
		CSY_DBG(
#else
		VDBG(cdev,
#endif
			"non-core control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);

		/* functions always handle their interfaces and endpoints...
		 * punt other recipients (other, WUSB, ...) to the current
		 * configuration code.
		 *
		 * REVISIT it could make sense to let the composite device
		 * take such requests too, if that's ever needed:  to work
		 * in config 0, etc.
		 */

		CSY_DBG("ctrl->bRequestType=0x%x, cdev->config=0x%p, intf=0x%x\n", ctrl->bRequestType, cdev->config, intf);
		switch (ctrl->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			if (cdev->config == NULL)
				return value;

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : RNDIS interface must be setted to 0.
 *                But sequence to set interface is depend on all functions list.
 *		  So, If you don't set RNDIS interface firstly in all functions list,
 *		  you have to change interface number to real rndis interface number from w_index.
 */
#  ifdef CSY_DBG
			for (i = 0; i < MAX_CONFIG_INTERFACES; i++) {
				f = cdev->config->interface[i];
				CSY_DBG("interface %d f=0x%p\n", i, f);
				if (!f)
					continue;
				CSY_DBG("interface %d = %s/%p disabled=%d\n", i, f->name, f, f->disabled);
			}
#  endif
			for (i = 0; i < MAX_CONFIG_INTERFACES; i++) {
				f = cdev->config->interface[i];
				if (!f)
					continue;

				/* RNDIS interface number must be changed to real interface number if rndis is enabled. */
				if ((f->disabled == 0) && (!strcmp(f->name, "rndis"))) {
					if ((i != intf) && (intf == 0)) {
						CSY_DBG("chaged intf to %d from %d (name=%s)\n", i, intf, f->name);
						intf = i;
					}
					break;
				}
			}
#endif
			f = cdev->config->interface[intf];
			CSY_DBG("f=0x%p, f->setup=0x%p, f->name=%s\n", f, f->setup, f->name);
			break;

		case USB_RECIP_ENDPOINT:
			endp = ((w_index & 0x80) >> 3) | (w_index & 0x0f);
			list_for_each_entry(f, &cdev->config->functions, list) {
				if (test_bit(endp, f->endpoints))
					break;
			}
			if (&f->list == &cdev->config->functions)
				f = NULL;
			break;
		}

		if (f && f->setup)
			value = f->setup(f, ctrl);
		else {
			struct usb_configuration	*c;

			c = cdev->config;
			if (c && c->setup) {
				CSY_DBG("ctrl->bRequest : default N:c->setup()\n");
				value = c->setup(c, ctrl);
		}
		}

		/* If the vendor request is not processed (value < 0),
		 * call all device registered configure setup callbacks
		 * to process it.
		 * This is used to handle the following cases:
		 * - vendor request is for the device and arrives before
		 * setconfiguration.
		 * - Some devices are required to handle vendor request before
		 * setconfiguration such as MTP, USBNET.
		 */

		if (value < 0) {
			struct usb_configuration        *cfg;

			list_for_each_entry(cfg, &cdev->configs, list) {
				if (cfg && cfg->setup) {
					CSY_DBG("vendor request is not processed :cfg->setup()\n");
				value = cfg->setup(cfg, ctrl);
			}
		}
		}

		goto done;
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		CSY_DBG("-respond with data transfer before status phase (req->length=%d, w_length=%d)\n", req->length,w_length);
		req->zero = value < w_length;
		value = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG(cdev, "ep_queue --> %d\n", value);
			req->status = 0;
			composite_setup_complete(gadget->ep0, req);
		}
	}

done:
	if(cdev->mute_switch)
		cdev->mute_switch = 0;
	CSY_DBG("--setup value=%d\n", value);
	/* device either stalls (value < 0) or reports success */
	return value;
}

static void composite_disconnect(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	unsigned long			flags;

	/* REVISIT:  should we have config and device level
	 * disconnect callbacks?
	 */
	spin_lock_irqsave(&cdev->lock, flags);
	if (cdev->config) {
		printk("composite_disconnect -> reset_config\n");
		reset_config(cdev);
	}

	if (cdev->mute_switch) {
	/* Replace below sequence (mute_switch value set 0),
           some times, disconnect is called more then one time.*/
//		cdev->mute_switch = 0;
		printk("composite_disconnect -> mute_switch\n");
	}
	else {
		schedule_work(&cdev->switch_work);
		printk("composite_disconnect -> switch_work\n");
	}
	spin_unlock_irqrestore(&cdev->lock, flags);
}

/*-------------------------------------------------------------------------*/

static ssize_t composite_show_suspended(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct usb_gadget *gadget = dev_to_usb_gadget(dev);
	struct usb_composite_dev *cdev = get_gadget_data(gadget);

	CSY_DBG("\n");
	return sprintf(buf, "%d\n", cdev->suspended);
}

static DEVICE_ATTR(suspended, 0444, composite_show_suspended, NULL);

static void
composite_unbind(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);

	/* composite_disconnect() must already have been called
	 * by the underlying peripheral controller driver!
	 * so there's no i/o concurrency that could affect the
	 * state protected by cdev->lock.
	 */
	WARN_ON(cdev->config);

	CSY_DBG("\n");
	while (!list_empty(&cdev->configs)) {
		struct usb_configuration	*c;

		c = list_first_entry(&cdev->configs,
				struct usb_configuration, list);
		while (!list_empty(&c->functions)) {
			struct usb_function		*f;

			f = list_first_entry(&c->functions,
					struct usb_function, list);
			list_del(&f->list);
			if (f->unbind) {
				DBG(cdev, "unbind function '%s'/%p\n",
						f->name, f);
				f->unbind(c, f);
				/* may free memory for "f" */
			}
		}
		list_del(&c->list);
		if (c->unbind) {
			DBG(cdev, "unbind config '%s'/%p\n", c->label, c);
			c->unbind(c);
			/* may free memory for "c" */
		}
	}
	if (composite->unbind)
		composite->unbind(cdev);

	if (cdev->req) {
		kfree(cdev->req->buf);
		usb_ep_free_request(gadget->ep0, cdev->req);
	}

	switch_dev_unregister(&cdev->sdev);
	kfree(cdev);
	set_gadget_data(gadget, NULL);
	device_remove_file(&gadget->dev, &dev_attr_suspended);
	composite = NULL;
}

static void
string_override_one(struct usb_gadget_strings *tab, u8 id, const char *s)
{
	struct usb_string		*str = tab->strings;

	for (str = tab->strings; str->s; str++) {
		if (str->id == id) {
			str->s = s;
			return;
		}
	}
}

static void
string_override(struct usb_gadget_strings **tab, u8 id, const char *s)
{
	while (*tab) {
		string_override_one(*tab, id, s);
		tab++;
	}
}

static void
composite_switch_work(struct work_struct *data)
{
	struct usb_composite_dev	*cdev =
		container_of(data, struct usb_composite_dev, switch_work);
	struct usb_configuration *config = cdev->config;

	printk("[composite_switch_work]config=0x%p\n",(void*)config);

	if (config)
		switch_set_state(&cdev->sdev, config->bConfigurationValue);
	else
		switch_set_state(&cdev->sdev, 0);
}

static int composite_bind(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev;
	int				status = -ENOMEM;

	CSY_DBG2("\n");
	cdev = kzalloc(sizeof *cdev, GFP_KERNEL);
	if (!cdev)
		return status;

	spin_lock_init(&cdev->lock);
	cdev->gadget = gadget;
	set_gadget_data(gadget, cdev);
	INIT_LIST_HEAD(&cdev->configs);

	/* preallocate control response and buffer */
	cdev->req = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
	if (!cdev->req)
		goto fail;
	cdev->req->buf = kmalloc(USB_BUFSIZ, GFP_KERNEL);
	if (!cdev->req->buf)
		goto fail;
	cdev->req->complete = composite_setup_complete;
	gadget->ep0->driver_data = cdev;

	cdev->bufsiz = USB_BUFSIZ;
	cdev->driver = composite;

	usb_gadget_set_selfpowered(gadget);

	/* interface and string IDs start at zero via kzalloc.
	 * we force endpoints to start unassigned; few controller
	 * drivers will zero ep->driver_data.
	 */
	usb_ep_autoconfig_reset(cdev->gadget);

	/* composite gadget needs to assign strings for whole device (like
	 * serial number), register function drivers, potentially update
	 * power state and consumption, etc
	 */
	status = composite->bind(cdev);
	if (status < 0)
		goto fail;

	cdev->sdev.name = "usb_configuration";
	status = switch_dev_register(&cdev->sdev);
	if (status < 0)
		goto fail;
	INIT_WORK(&cdev->switch_work, composite_switch_work);

	cdev->desc = *composite->dev;
	cdev->desc.bMaxPacketSize0 = gadget->ep0->maxpacket;

	/* standardized runtime overrides for device ID data */
	if (idVendor)
		cdev->desc.idVendor = cpu_to_le16(idVendor);
	if (idProduct)
		cdev->desc.idProduct = cpu_to_le16(idProduct);
	if (bcdDevice)
		cdev->desc.bcdDevice = cpu_to_le16(bcdDevice);

	/* strings can't be assigned before bind() allocates the
	 * releavnt identifiers
	 */
	if (cdev->desc.iManufacturer && iManufacturer)
		string_override(composite->strings,
			cdev->desc.iManufacturer, iManufacturer);
	if (cdev->desc.iProduct && iProduct)
		string_override(composite->strings,
			cdev->desc.iProduct, iProduct);
	if (cdev->desc.iSerialNumber && iSerialNumber)
		string_override(composite->strings,
			cdev->desc.iSerialNumber, iSerialNumber);

	status = device_create_file(&gadget->dev, &dev_attr_suspended);
	if (status)
		goto fail;

	INFO(cdev, "%s ready\n", composite->name);
	return 0;

fail:
	composite_unbind(gadget);
	return status;
}

/*-------------------------------------------------------------------------*/

static void
composite_suspend(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_function		*f;

	/* REVISIT:  should we have config level
	 * suspend/resume callbacks?
	 */
	DBG(cdev, "suspend\n");
	if (cdev->config) {
		list_for_each_entry(f, &cdev->config->functions, list) {
			if (f->suspend)
				f->suspend(f);
		}
	}
	if (composite->suspend)
		composite->suspend(cdev);

	cdev->suspended = 1;
}

static void
composite_resume(struct usb_gadget *gadget)
{
	struct usb_composite_dev	*cdev = get_gadget_data(gadget);
	struct usb_function		*f;

	/* REVISIT:  should we have config level
	 * suspend/resume callbacks?
	 */
	DBG(cdev, "resume\n");
	if (composite->resume)
		composite->resume(cdev);
	if (cdev->config) {
		list_for_each_entry(f, &cdev->config->functions, list) {
			if (f->resume)
				f->resume(f);
		}
	}

	cdev->suspended = 0;
}

static int
composite_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct usb_function *f = dev_get_drvdata(dev);

	CSY_DBG2("\n");
	if (!f) {
		/* this happens when the device is first created */
		return 0;
	}

	if (add_uevent_var(env, "FUNCTION=%s", f->name))
		return -ENOMEM;
	if (add_uevent_var(env, "ENABLED=%d", !f->disabled))
		return -ENOMEM;
	return 0;
}

/*-------------------------------------------------------------------------*/

static struct usb_gadget_driver composite_driver = {
	.speed		= USB_SPEED_HIGH,

	.bind		= composite_bind,
	.unbind		= composite_unbind,

	.setup		= composite_setup,
	.disconnect	= composite_disconnect,

	.suspend	= composite_suspend,
	.resume		= composite_resume,

	.driver	= {
		.owner		= THIS_MODULE,
	},
};

/**
 * usb_composite_register() - register a composite driver
 * @driver: the driver to register
 * Context: single threaded during gadget setup
 *
 * This function is used to register drivers using the composite driver
 * framework.  The return value is zero, or a negative errno value.
 * Those values normally come from the driver's @bind method, which does
 * all the work of setting up the driver to match the hardware.
 *
 * On successful return, the gadget is ready to respond to requests from
 * the host, unless one of its components invokes usb_gadget_disconnect()
 * while it was binding.  That would usually be done in order to wait for
 * some userspace participation.
 */
int usb_composite_register(struct usb_composite_driver *driver)
{
	CSY_DBG2("\n");
	if (!driver || !driver->dev || !driver->bind || composite)
		return -EINVAL;

	if (!driver->name)
		driver->name = "composite";
	composite_driver.function =  (char *) driver->name;
	composite_driver.driver.name = driver->name;
	composite = driver;

	driver->class = class_create(THIS_MODULE, "usb_composite");
	if (IS_ERR(driver->class))
		return PTR_ERR(driver->class);
	driver->class->dev_uevent = composite_uevent;

	return usb_gadget_register_driver(&composite_driver);
}

/**
 * usb_composite_unregister() - unregister a composite driver
 * @driver: the driver to unregister
 *
 * This function is used to unregister drivers using the composite
 * driver framework.
 */
void usb_composite_unregister(struct usb_composite_driver *driver)
{
	CSY_DBG2("\n");
	if (composite != driver)
		return;
	usb_gadget_unregister_driver(&composite_driver);
}
