#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "qemu/typedefs.h"
#include "hw/virtio/virtio-mmc.h"
#include "qemu/iov.h"
#include <stdint.h>
#include <stdio.h>

typedef struct virtio_mmc_req {
	uint32_t opcode;
	uint32_t arg;
	uint32_t flags;
	uint32_t blocks;
	uint32_t blksz;
} virtio_mmc_req;

static void handle_mmc_request(VirtIODevice *vdev, virtio_mmc_req *req, uint8_t *response) {
    printf("[mmcpcidebug] virtio-mmc.c: handle_mmc_request called\n");

    VirtIOMMC *vmmc = VIRTIO_MMC(vdev);

    SDRequest sdreq;
    sdreq.cmd = (uint8_t)req->opcode;
    sdreq.arg = req->arg;

    printf("[mmcpcidebug] qemu, virtio-mmc.c: sdreq.cmd = %d, arg = %d\n", sdreq.cmd, sdreq.arg);
    
    sdbus_do_command(&vmmc->sdbus, &sdreq, response);

    printf("[mmcpcidebug] qemu, virtio-mmc.c: response = %d\n", *response);
}

static void handle_input(VirtIODevice *vdev, VirtQueue *vq) {
    printf("[mmcpcidebug] virtio-mmc.c: handle_input called\n");

    VirtQueueElement *elem;
    virtio_mmc_req data;

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));

    iov_to_buf(elem->out_sg, elem->out_num, 0, &data, sizeof(virtio_mmc_req));

    uint8_t response;
    handle_mmc_request(vdev, &data, &response);

    iov_from_buf(elem->in_sg, elem->in_num, 0, &response, sizeof(uint8_t));

    virtqueue_push(vq, elem, 1);

    virtio_notify(vdev, vq);
}

static void virtio_mmc_realize(DeviceState *dev, Error **errp) {
    printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_realize called\n");
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOMMC *vmmc = VIRTIO_MMC(dev);

    printf("[mmcpcidebug] virtio-mmc.c: VIRTIO_ID_MMC = %d\n", VIRTIO_ID_MMC);
    virtio_init(vdev, VIRTIO_ID_MMC, 0);

    vmmc->vq = virtio_add_queue(vdev, 1, handle_input);

    qbus_init(&vmmc->sdbus, sizeof(vmmc->sdbus), TYPE_SD_BUS, dev, "sd-bus");

    printf("[mmcpcidebug] virtio-mmc.c: sdbus inserted before setting is %d\n", sdbus_get_inserted(&vmmc->sdbus));
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
