#include "rtl8139.h"

#include "../../device/pci/pci.h"
#include "../../interrupt/intr.h"
#include "../../memory/mem.h"
#include "common/cpu/contrl.h"
#include "common/cpu/mio.h"
#include "common/lib/string.h"
#include "common/tool/log.h"
#include "driver.h"
#include "../util.h"


static rtl8139_priv_t *priv;
netif_t *netif;

pci_device_t *find_rtl8139_device() { return pci_find_device(0x10ec, 0x8139); }

void rtl8139_reset(rtl8139_priv_t *device) {
    uint32_t io_base = device->ioaddr;
    outb(io_base + CR, CR_RST);  // 复位命令
    for (int i = 100000; i > 0; i--) {
        if (!((inb(io_base + CR) & CR_RST))) {
            log_debug("rtl8139 reset complete\n");
            return;
        }
    }
}

void rtl8139_setup_tx(rtl8139_priv_t *device) {
    // uint32_t io_base = device->ioaddr;
    // uint8_t cr = inb(io_base);
    // outb(+CR, cr | CR_TE);
    // outb(io_base + TCR, cr | TCR_IFG_96 | TCR_MXDMA_1024);

    // for (int i = 0; i < R8139DN_TX_DESC_NB; i++) {
    //     uint32_t addr =
    //         (uint32_t)(device->tx.buffer + i * R8139DN_TX_DESC_SIZE);
    //     device->tx.data[i] = (uint8_t *)addr;
    //     outl(io_base + TSAD0 + i + TSAD1 - TSAD0, addr);
    //     device->tx.curr = device->tx.dirty = 0;
    //     device->tx.free_count = R8139DN_TX_DESC_NB;
    // }

    uint8_t cr = inb(priv->ioaddr + CR);

    // 打开发送允许位
    outb(priv->ioaddr + CR, cr | CR_TE);

    // 配置发送相关的模型，960ns用于100mbps，添加CRC，DMA 1024B
    outl(priv->ioaddr + TCR,  TCR_IFG_96 | TCR_MXDMA_1024);

    // 配置发送缓存,共4个缓冲区，并设备各个缓冲区首地址TSAD0-TSAD3
    for (int i = 0; i < R8139DN_TX_DESC_NB ; i++) {
        uint32_t addr = (uint32_t)(priv->tx.buffer + i * R8139DN_TX_DESC_SIZE);
        priv->tx.data[i] = (uint8_t *)addr;
        outl(priv->ioaddr + TSAD0 + i * TSAD_GAP, addr);
    }

    // 指向第0个缓存包
    priv->tx.curr = priv->tx.dirty = 0;
    priv->tx.free_count = R8139DN_TX_DESC_NB;
}

void rtl8139_setup_rx(rtl8139_priv_t *device) {
    // uint32_t io_base = device->ioaddr;
    // uint8_t cr = inb(io_base + CR);
    // outb(io_base + CR, cr & ~CR_RE);

    // device->rx.curr = 0;

    // device->rx.data = device->rx.buffer;

    // // Multiple Interrupt Select Register?
    // outw(io_base + MULINT, 0);

    // outl(io_base + RBSTART, (uint32_t)device->rx.data);

    // outl(io_base + RCR,
    //      RCR_MXDMA_1024 | RCR_APM | RCR_AB | RCR_RBLEN_16K | RCR_WRAP);

    // cr = inb(io_base + CR);
    // outb(io_base + CR, cr | CR_RE);
     //  先禁止接上
    uint8_t cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr & ~CR_RE );

    priv->rx.curr = 0;

    // 设置接收缓存
    priv->rx.data = priv->rx.buffer;

    // Multiple Interrupt Select Register?
    outw(priv->ioaddr + MULINT, 0);

    // 接收缓存起始地址
    outl(priv->ioaddr + RBSTART, (uint32_t)priv->rx.data);

    // 接收配置：广播+单播匹配、DMA bust 1024？接收缓存长16KB+16字节
    outl(priv->ioaddr + RCR, RCR_MXDMA_1024 | RCR_APM | RCR_AB | RCR_RBLEN_16K | RCR_WRAP);

    // 允许接收数据
    cr = inb(priv->ioaddr + CR);
    outb(priv->ioaddr + CR, cr | CR_RE );
}

void pic_send_eoi(int irq_num) {
    #define IRQ_PIC_START		0x20			// PIC中断起始号
    #define PIC1_OCW2			0xa0
    #define PIC_OCW2_EOI		(1 << 5)		// 1 - 非特殊结束中断EOI命令
    #define PIC0_OCW2			0x20
    irq_num -= IRQ_PIC_START;

    // 从片也可能需要发送EOI
    if (irq_num >= 8) {
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    }

    outb(PIC0_OCW2, PIC_OCW2_EOI);
}
void handle_rtl8139(int intr_no) {
    // 先读出所有中断，然后清除中断寄存器的状态
    uint16_t isr = inw(priv->ioaddr + ISR);
    outw(priv->ioaddr + ISR, 0xFFFF);

    // 先处理发送完成和错误
    if ((isr & INT_TOK) || (isr & INT_TER)) {

        // 尽可能取出多的数据包，然后将缓存填满
        while (priv->tx.free_count < 4) {
            // 取dirty位置的发送状态, 如果发送未完成，即无法写了，那么就退出
            uint32_t tsd =
                inl(priv->ioaddr + TSD0 + (priv->tx.dirty * TSD_GAP));
            if (!(tsd & (TSD_TOK | TSD_TABT | TSD_TUN))) {
                break;
            }

            // 增加计数
            priv->tx.free_count++;
            if (++priv->tx.dirty >= R8139DN_TX_DESC_NB) {
                priv->tx.dirty = 0;
            }

            // 尽可能取出多的数据包，然后将缓存填满
            pktbuf_t *buf = netif_get_out(netif, -1);
            if (buf) {
                priv->tx.free_count--;

                // 写入发送缓存，不必调整大小，上面已经调整好
                int len = buf->total_size;
                pktbuf_read(buf, priv->tx.data[priv->tx.curr], len);
                pktbuf_free(buf);

                // 启动整个发送过程, 只需要写TSD，TSAD已经在发送时完成
                // 104 bytes of early TX
                // threshold，104字节才开始发送??，以及发送的包的字节长度，清除OWN位
                uint32_t tx_flags = (1 << TSD_ERTXTH_SHIFT) | len;
                outl(priv->ioaddr + TSD0 + priv->tx.curr * TSD_GAP, tx_flags);

                // 取下一个描述符f地址
                priv->tx.curr += 1;
                if (priv->tx.curr >= R8139DN_TX_DESC_NB) {
                    priv->tx.curr = 0;
                }
            }
        }
    }

    // 对于接收的测试，可以ping本地广播来测试。否则windows上，ping有时会不发包，而是要过一段时间，难得等
    if (isr & INT_ROK) {
        // 循环读取，直到缓存为空
        while ((inb(priv->ioaddr + CR) & CR_BUFE) == 0) {
            rtl8139_rpkthdr_t *hdr =
                (rtl8139_rpkthdr_t *)(priv->rx.data + priv->rx.curr);
            int pkt_size = hdr->size - CRC_LEN;

            // 进行一些简单的错误检查，有问题的包就不要写入接收队列了
            if (hdr->FAE || hdr->CRC || hdr->LONG || hdr->RUNT || hdr->ISE) {
                // 重新对接收进行设置，重启。还是忽略这个包，还是忽略吧
                rtl8139_setup_rx(priv);
            } else if ((pkt_size < 0) || (pkt_size >= R8139DN_MAX_ETH_SIZE)) {
                // 重新对接收进行设置，重启。还是忽略这个包，还是忽略吧
                rtl8139_setup_rx(priv);
            } else {
                // 分配包缓存空间
                pktbuf_t *buf = pktbuf_alloc(pkt_size);
                if (buf) {
                    // 这里要考虑跨包拷贝的情况
                    if (priv->rx.curr + hdr->size > R8139DN_RX_DMA_SIZE) {
                        // 分割了两块空间
                        int count = R8139DN_RX_DMA_SIZE - priv->rx.curr -
                                    sizeof(rtl8139_rpkthdr_t);
                        pktbuf_write(buf,
                                     priv->rx.data + priv->rx.curr +
                                         sizeof(rtl8139_rpkthdr_t),
                                     count);  // 第一部分到尾部
                        pktbuf_write(buf, priv->rx.data, pkt_size - count);
                    } else {
                        // 跳过开头的4个字节的接收状态头
                        pktbuf_write(buf,
                                     priv->rx.data + priv->rx.curr +
                                         sizeof(rtl8139_rpkthdr_t),
                                     pkt_size);
                    }

                    // 将收到的包发送队列
                    netif_put_in(netif, buf, -1);
                } else {
                    // 包不够则丢弃
                }

                // 移至下一个包
                priv->rx.curr = (priv->rx.curr + hdr->size +
                                 sizeof(rtl8139_rpkthdr_t) + 3) &
                                ~3;  // 4字节对齐

                // 调整curr在整个有效的缓存空间内
                if (priv->rx.curr > R8139DN_RX_BUFLEN) {
                    priv->rx.curr -= R8139DN_RX_BUFLEN;
                }

                // 更新接收地址，通知网卡可以继续接收数据
                outw(priv->ioaddr + CAPR, priv->rx.curr - 16);
            }
        }
    }
    // 别忘了这个，不然中断不会响应
    pic_send_eoi(intr_no);
    
    log_debug("net intr_from:%d\n", intr_no);
}

net_err_t rtl8139_open(struct _netif_t *p_netif, void *ops_data) {
    priv = (rtl8139_priv_t *)ops_data;
    netif = p_netif;

    pci_device_t *p_device = find_rtl8139_device();
    if (p_device == NULL) {
        log_err("rtl8139 not foud");
        return NET_ERR_NONE;
    }
    priv->bus = p_device->bus;
    priv->device = p_device->device_id;
    priv->func = p_device->func;
    pci_enable_bus(p_device);
    pci_bar_t bar;
    pci_find_bar(p_device, &bar, PCI_BAR_TYPE_IO);
    priv->ioaddr = bar.io_base;
    log_debug("rtl8139 io_base=%d\n", priv->ioaddr);
    uint8_t intr_no = pci_interrupt(p_device);
    // map_mem_direct(bar.io_base, bar.io_base, bar.size);
    outb(bar.io_base + 0x52, 0);
    outb(bar.io_base + 0x52, 0x0);
    for (int i = 0; i < 6; i++) {
        netif->mac[i] = inb(bar.io_base + i);
    }
    netif->type = NETIF_TYPE_ETHER;
    char *macStr = "aa:bb:cc:dd:ee:ff";
    mac2str(netif->mac, macStr);
    log_debug("mac:%s\n", macStr);
    rtl8139_reset(priv);
    rtl8139_setup_tx(priv);
    rtl8139_setup_rx(priv);

    intr_disable(intr_no);
    outw(priv->ioaddr + IMR, 0xFFFF);
    register_intr_handler(intr_no, handle_rtl8139);
    intr_enable(intr_no);
    return NET_ERR_OK;
}

net_err_t rtl8139_xmit(struct _netif_t *netif) {
    // 如果发送缓存已经满，即写入比较快，退出，让中断自行处理发送过程
    // if (priv->tx.free_count <= 0) {
    // return NET_ERR_OK;
    // }

    // 取数据包
    pktbuf_t *buf = netif_get_out(netif, 0);

    // 禁止响应中断，以便进行一定程序的保护
    eflags_t state = enter_intr_protect();

    // 写入发送缓存，不必调整大小，上面已经调整好
    int len = buf->total_size;
    pktbuf_read(buf, priv->tx.data[priv->tx.curr], len);
    pktbuf_free(buf);

// 启动整个发送过程, 只需要写TSD，TSAD(数据地址和长度)已经在发送时完成
// 104 bytes of early TX
// threshold，104字节才开始发送??，以及发送的包的字节长度，清除OWN位
    uint32_t tx_flags = (1 << TSD_ERTXTH_SHIFT) | len;
    outl(priv->ioaddr + TSD0 + priv->tx.curr * TSD_GAP, tx_flags);

    // 取下一个描述符f地址
    priv->tx.curr += 1;
    if (priv->tx.curr >= R8139DN_TX_DESC_NB) {
        priv->tx.curr = 0;
    }

    // 减小空闲计数
    priv->tx.free_count--;

    leave_intr_protect(state);
    return NET_ERR_OK;
}