#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "qemu/typedefs.h"
#include "hw/virtio/virtio-mmc.h"
#include "qemu/iov.h"
#include <stdint.h>

static void handle_input(VirtIODevice *vdev, VirtQueue *vq) {
    printf("[mmcpcidebug] virtio-mmc.c: handle_input called\n");

    VirtQueueElement *elem;
    uint32_t data;

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));

    iov_to_buf(elem->out_sg, elem->out_num, 0, &data, sizeof(uint32_t));

    data *= data;

    iov_from_buf(elem->in_sg, elem->in_num, 0, &data, sizeof(uint32_t));

    virtqueue_push(vq, elem, 1);
    virtio_notify(vdev, vq);
}

static void virtio_mmc_realize(DeviceState *dev, Error **errp) {
    printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_realize called\n");
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOMMC *vmmc = VIRTIO_MMC(dev);

    virtio_init(vdev, VIRTIO_ID_MMC, 0);

    vmmc->vq = virtio_add_queue(vdev, 1, handle_input);
}

static void virtio_mmc_unrealize(DeviceState *dev) {
    printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_unrealize called\n");
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    virtio_cleanup(vdev);
}

static uint64_t virtio_mmc_get_features(VirtIODevice *vdev, uint64_t features, Error **errp) {
    return features;
}

static void virtio_mmc_class_init(ObjectClass *klass, void *data) {
    printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_class_init called\n");
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
    printf("[mmcpcidebug] virtio-mmc.c: virtio_register_types called\n");
    type_register_static(&virtio_mmc_info);
}

type_init(virtio_register_types)
