#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-mmc.h"

static void handle_input(VirtIODevice *vdev, VirtQueue *vq) {
    virtio_notify(vdev, vq);
}

static void virtio_mmc_realize(DeviceState *dev, Error **errp) {
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOMMC *vmmc = VIRTIO_MMC(dev);

    virtio_init(vdev, VIRTIO_ID_MMC, 0);

    vmmc->vq = virtio_add_queue(vdev, 1, handle_input);
}

static void virtio_mmc_unrealize(DeviceState *dev) {

}

static uint64_t virtio_mmc_get_features(VirtIODevice *vdev, uint64_t features, Error **errp) {
    return features;
}

static void virtio_mmc_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *k = VIRTIO_DEVICE_CLASS(klass);

    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    k->realize = virtio_mmc_realize;
    k->unrealize = virtio_mmc_unrealize;
    k->get_features = virtio_mmc_get_features;
}

static const TypeInfo virtio_mmc_info = {
    .name = TYPE_VIRTIO_MMC,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOMMC),
    .class_init = virtio_mmc_class_init,
};

static void virtio_register_types(void)
{
    type_register_static(&virtio_mmc_info);
}

type_init(virtio_register_types)
