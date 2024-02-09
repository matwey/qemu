#include "qemu/osdep.h"

#include "hw/virtio/virtio-pci.h"
#include "hw/virtio/virtio-mmc.h"

typedef struct VirtIOMMCPCI VirtIOMMCPCI;

/*
 * virtio-mmc-pci: This extends VirtioPCIProxy.
 */
#define TYPE_VIRTIO_MMC_PCI "virtio-mmc-pci-base"
DECLARE_INSTANCE_CHECKER(VirtIOMMCPCI, VIRTIO_MMC_PCI,
                         TYPE_VIRTIO_MMC_PCI)

struct VirtIOMMCPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOMMC vdev;
};

static void virtio_mmc_pci_instance_init(Object *obj)
{
    VirtIOMMCPCI *dev = VIRTIO_MMC_PCI(obj);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_MMC);
}

static void virtio_mmc_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOMMCPCI *vmmc = VIRTIO_MMC_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&vmmc->vdev);

    qdev_set_parent_bus(vdev, BUS(&vpci_dev->bus), errp);
    // if (!qdev_realize(vdev, BUS(&vpci_dev->bus), errp)) {
    //     printf("[mmcpcidebug] Virtio MMC PCI not realized\n");
    //     return;
    // }

    virtio_pci_force_virtio_1(vpci_dev);
    object_property_set_bool(OBJECT(vdev), "realized", true, errp);
    printf("[mmcpcidebug] Virtio MMC PCI realized\n");
}

static void virtio_mmc_pci_class_init(ObjectClass *klass, void *data)
{
    printf("[mmcpcidebug] virtio_mmc_pci_class_init called\n");
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);

    k->realize = virtio_mmc_pci_realize;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    pcidev_k->vendor_id = 0xabcd;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static const VirtioPCIDeviceTypeInfo virtio_mmc_pci_info = {
    .base_name     = TYPE_VIRTIO_MMC_PCI,
    .generic_name  = "virtio-mmc-pci",
    .instance_size = sizeof(VirtIOMMCPCI),
    .instance_init = virtio_mmc_pci_instance_init,
    .class_init    = virtio_mmc_pci_class_init,
};

static void virtio_mmc_pci_register(void)
{
    virtio_pci_types_register(&virtio_mmc_pci_info);
}

type_init(virtio_mmc_pci_register)