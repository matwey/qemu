#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "qemu/typedefs.h"
#include "hw/virtio/virtio-mmc.h"
#include "qemu/iov.h"
#include "sysemu/block-backend-global-state.h"
#include "hw/sd/sdcard_legacy.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct virtio_mmc_req {
    bool is_request;
	uint32_t opcode;
	uint32_t arg;
	uint32_t flags;
	uint32_t blocks;
	uint32_t blksz;
    bool is_data;
    bool is_write;

    bool is_set_ios;
    uint16_t vdd;
} virtio_mmc_req;

typedef struct virtio_mmc_resp {
	uint32_t response[4];
    int resp_len;
    uint8_t buf[1024];
} virtio_mmc_resp;

static void handle_mmc_request(VirtIODevice *vdev, virtio_mmc_req *req, virtio_mmc_resp *response) {
    printf("[mmcpcidebug] virtio-mmc.c: handle_mmc_request called\n");

    VirtIOMMC *vmmc = VIRTIO_MMC(vdev);

    if(req->is_request) {
        SDRequest sdreq;
        sdreq.cmd = (uint8_t)req->opcode;
        sdreq.arg = req->arg;
        // sdreq.crc = (uint8_t)req->flags;
        printf("[mmcpcidebug] virtio-mmc.c: sdreq.cmd = %d, arg = %x\n", sdreq.cmd, sdreq.arg);
        int resp_len = sd_do_command(vmmc->sd, &sdreq, (uint8_t*)response->response);
        response->resp_len = resp_len;

        // fix response: 0xaa010000 -> 0x000001aa
        for(int i=0;i<resp_len/4;i++) {
            uint32_t temp = response->response[i];
            response->response[i] = ((temp & 0xff) << 24) | ((temp & 0xff00) << 8) | ((temp & 0xff0000) >> 8) | ((temp & 0xff000000) >> 24);
        }

        // vvv was used when response was u8 instead of u32 vvv
        // // reverse the response (because for some reason (probably endianness) 
        // // the response is reversed in the sd_do_command function)
        // for(int i = 0; i < 2; i++) {
        //     uint8_t temp = response->response[i];
        //     response->response[i] = response->response[3-i];
        //     response->response[3-i] = temp;
        // }

        printf("[mmcpcidebug] resp_len = %d; response: ", resp_len);
        for(int i=0;i<resp_len/4;i++) {
            printf("%x, ", response->response[i]);
        }
        printf("\n");

        if(req->is_data){
            for(uint32_t i=0;i<req->blocks;i++) {
                for(int j=0;j<req->blksz;j++) {
                    if(req->is_write){
                        sd_write_byte(vmmc->sd, response->buf[i*req->blksz+j]);
                    } else {
                        response->buf[i*req->blksz+j] = sd_read_byte(vmmc->sd);
                    }
                }
            }
        }
    } else if(req->is_set_ios) {
        printf("[mmcpcidebug] virtio-mmc.c: setting voltage = %d\n", req->vdd);
    }
}

static void handle_input(VirtIODevice *vdev, VirtQueue *vq) {
    printf("[mmcpcidebug] virtio-mmc.c: handle_input called\n");

    VirtQueueElement *elem;
    virtio_mmc_req data;

    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));

    iov_to_buf(elem->out_sg, elem->out_num, 0, &data, sizeof(virtio_mmc_req));

    virtio_mmc_resp response;
    handle_mmc_request(vdev, &data, &response);

    iov_from_buf(elem->in_sg, elem->in_num, 0, &response, sizeof(virtio_mmc_resp));

    virtqueue_push(vq, elem, 1);

    virtio_notify(vdev, vq);
}

static void virtio_mmc_virtual_queue_init(VirtIODevice *vdev, VirtIOMMC *vmmc) {
    // printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_virtual_queue_init called\n");
    vmmc->vq = virtio_add_queue(vdev, 1, handle_input);
}

static void print_response(uint8_t *response) {
    for(int i = 0; i < 4; i++) {
        printf("[mmcpcidebug] virtio-mmc.c: response[%d] = %d\n", i, response[i]);
    }
}

static void do_testing_stuff(SDState *sd) {
    return;
    printf("[mmcpcidebug] virtio-mmc.c: do_testing_stuff called\n");

    uint8_t response[4]={0};
    SDRequest request;
    request.arg = 0;
    request.crc = 0;

    request.cmd = 0;
    sd_do_command(sd, &request, response);
    printf("[mmcpcidebug] virtio-mmc.c: 1) response = %d\n", response);
    print_response(response);

    request.cmd = 8;
    request.arg = 0x1AA;
    sd_do_command(sd, &request, response);
    printf("[mmcpcidebug] virtio-mmc.c: 2) response = %d\n", response);
    print_response(response);
    


}

static void virtio_mmc_realize(DeviceState *dev, Error **errp) {
    printf("[mmcpcidebug] virtio-mmc.c: virtio_mmc_realize called\n");
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOMMC *vmmc = VIRTIO_MMC(dev);

    printf("[mmcpcidebug] virtio-mmc.c: VIRTIO_ID_MMC = %d\n", VIRTIO_ID_MMC);
    virtio_init(vdev, VIRTIO_ID_MMC, 0);

    virtio_mmc_virtual_queue_init(vdev, vmmc);

    BlockBackend *blk = blk_by_name("my_mmc");
    if(!blk) {
        printf("[mmcpcidebug] virtio-mmc.c: blk_by_name failed\n");
        return;
    }
    vmmc->sd = sd_init(blk, false);
    if(!vmmc->sd) {
        printf("[mmcpcidebug] virtio-mmc.c: sd_init failed\n");
        return;
    }
    printf("[mmcpcidebug] virtio-mmc.c: sd_init success\n");


    do_testing_stuff(vmmc->sd);
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
