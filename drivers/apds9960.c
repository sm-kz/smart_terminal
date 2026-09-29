#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/ide.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/input.h>
#include <asm/bitops.h>
#include <linux/miscdevice.h>

#define IOCTL_GET_ALS           _IOR('A', 0, struct als_data)
#define IOCTL_GET_PROXIMITY     _IOR('A', 1, struct proximity_data)
#define IOCTL_CALIBRATE         _IO('A', 2)

/* ========== 寄存器定义 ==========*/
#define REG_ENABLE      0x80
#define REG_ATIME       0x81
#define REG_WTIME       0x83
#define REG_AILTL       0x84
#define REG_AILTH       0x85
#define REG_AIHTL       0x86
#define REG_AIHTH       0x87
#define REG_PILT        0x89
#define REG_PIHT        0x8B
#define REG_PERS        0x8C
#define REG_CONFIG1     0x8D
#define REG_PPULSE      0x8E
#define REG_CONTROL     0x8F
#define REG_CONFIG2     0x90
#define REG_ID          0x92
#define REG_STATUS      0x93
#define REG_CDATAL      0x94
#define REG_CDATAH      0x95
#define REG_RDATAL      0x96
#define REG_RDATAH      0x97
#define REG_GDATAL      0x98
#define REG_GDATAH      0x99
#define REG_BDATAL      0x9A
#define REG_BDATAH      0x9B
#define REG_PDATA       0x9C
#define REG_POFFSET_UR  0x9D
#define REG_POFFSET_DL  0x9E
#define REG_CONFIG3     0x9F
#define REG_GPENTH      0xA0
#define REG_GEXTH       0xA1
#define REG_GCONF1      0xA2
#define REG_GCONF2      0xA3
#define REG_GOFFSET_U   0xA4
#define REG_GOFFSET_D   0xA5
#define REG_GOFFSET_L   0xA7
#define REG_GOFFSET_R   0xA9
#define REG_GPULSE      0xA6
#define REG_GCONF3      0xAA
#define REG_GCONF4      0xAB
#define REG_GFLVL       0xAE
#define REG_GSTATUS     0xAF
#define REG_IFORCE      0xE4
#define REG_PICLEAR     0xE5
#define REG_CICLEAR     0xE6
#define REG_AICLEAR     0xE7
#define REG_GFIFO_U     0xFC
#define REG_GFIFO_D     0xFD
#define REG_GFIFO_L     0xFE
#define REG_GFIFO_R     0xFF

/* =================== 寄存器位域掩码 =================== */
// REG_ENABLE
#define GEN          (1 << 6)
#define PIEN         (1 << 5)
#define AIEN         (1 << 4)
#define WEN          (1 << 3)
#define PEN          (1 << 2)
#define AEN          (1 << 1)
#define PON          (1 << 0)
// REG_STATUS
#define PINT         (1 << 5)
#define AINT         (1 << 4)
#define GINT         (1 << 2)
#define PVALID       (1 << 1)
#define AVALID       (1 << 0)
// REG_GCONFIG4
#define GFIFO_CLR    (1 << 2)
#define GIEN         (1 << 1)
#define GMODE        (1 << 0)
// REG_CONTROL
#define PGAIN_MASK   0x0c
#define AGAIN_MASK   0x03
#define LDRIVE_MASK  0xc0
// REG_GSTATUS
#define GVALID       (1 << 0)
#define GFOV         (1 << 1)

/* ================== 默认配置 ================*/
#define ALS_OFF_TH          0x00a0
#define PROXIMITY_OFF_TH    0x20
#define ALS_PROX_SAMPLE_PERIOD      300
#define IOCTL_TIMEOUT               1000

struct reg_default als_threshold_default[] = {
    {REG_ATIME, 0xf0},
    {REG_AILTL, 0x00},
    {REG_AILTH, 0x00},
    {REG_AIHTL, ALS_OFF_TH & 0xff},
    {REG_AIHTH, ALS_OFF_TH >> 8}
};

struct reg_default proximity_threshold_default[] = {
    {REG_PPULSE, 0x85}, // 决定脉冲数量和时长，太短或太少容易漏检
    {REG_PILT, 0x00},   // 远离时不产生中断
    {REG_PIHT, PROXIMITY_OFF_TH}    // 靠近判断阈值设置的低一点
};

struct reg_default gesture_threshold_default[] = {
    {REG_GCONF1, 0x80},    // FIFO 阈值为4
    {REG_GCONF2, 0x2d},     // 驱动能力不要太强，避免溢出
    {REG_GPENTH, 0x20},     // Gesture 模式进入阈值（高于）
    {REG_GEXTH, 0x40}       // Gesture 模式退出阈值（低于）
};

struct regmap_config regmap_cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .cache_type = REGCACHE_NONE
};

struct gesture_data {
    int first_ud;
    int first_lr;
    int last_ud;
    int last_lr;
    int ud_in_process;
    int lr_in_process;
};

struct als_data {
    u16 c;
    u16 r;
    u16 g;
    u16 b;
};

struct proximity_data {
    u8 dist;
};

struct apds9960_data {
    struct i2c_client *client;
    struct regmap *regmap;
    struct work_struct work;    // 中断任务 bh

    struct gesture_data gd;
    struct als_data ad;
    struct proximity_data pd;
    int proximity_enabled;
    int als_enabled;
    atomic_t ad_ready;
    atomic_t pd_ready;

    struct fasync_struct *fa;   // input 异步上报
    wait_queue_head_t wq;

    // proximity, ALS 数据定时读取，不依赖中断
    struct delayed_work proximity_work;
    struct delayed_work als_work;

    struct input_dev *input_dev;
    struct miscdevice misc;
};

static int apds9960_fopen (struct inode *inode, struct file *filp) {
    struct apds9960_data *apds9960;
    struct miscdevice *misc;
    // misc 设备的 filp 的私有数据默认为 misc 设备指针
    misc = filp->private_data;
    apds9960 = container_of(misc, struct apds9960_data, misc);
    filp->private_data = apds9960;
    return 0;
}

static int apds9960_fclose (struct inode *inode, struct file *filp) {
    struct apds9960_data *apds9960;
    apds9960 = filp->private_data;
    filp->private_data = &apds9960->misc;

    apds9960->proximity_enabled = 0;
    apds9960->als_enabled = 0;
    cancel_delayed_work_sync(&apds9960->proximity_work);
    cancel_delayed_work_sync(&apds9960->als_work);

    return 0;
}

static int apds9960_calibration(struct apds9960_data *apds9960);

static long apds9960_ioctl (struct file *filp, unsigned int code, unsigned long data) {
    struct apds9960_data *apds9960 = filp->private_data;
    int ret;

    switch (code) {
    case IOCTL_GET_ALS:
        ret = wait_event_interruptible_timeout(apds9960->wq, 
                                               atomic_read(&apds9960->ad_ready),
                                               msecs_to_jiffies(IOCTL_TIMEOUT));
        if (ret < 0)
            return ret;
        else if (ret == 0)
            return -ETIMEDOUT;

        if (copy_to_user((void __user *)data, &apds9960->ad, sizeof(struct als_data)) > 0)
            return -EFAULT;
        atomic_set(&apds9960->ad_ready, 0);
        return 0;

        break;
    case IOCTL_GET_PROXIMITY:
        ret = wait_event_interruptible_timeout(apds9960->wq, 
                                               atomic_read(&apds9960->pd_ready),
                                               msecs_to_jiffies(IOCTL_TIMEOUT));
        if (ret < 0)
            return ret;
        else if (ret == 0)
            return -ETIMEDOUT;

        if (copy_to_user((void __user *)data, &apds9960->pd, sizeof(struct proximity_data)) > 0)
            return -EFAULT;
        atomic_set(&apds9960->pd_ready, 0);
        return 0;

        break;
    case IOCTL_CALIBRATE:
        apds9960_calibration(apds9960);
        return 0;
        break;
    default: 
        return -ENOTTY;
        break;
    }
    return 0;
}

struct file_operations apds9960_fops = {
    .owner = THIS_MODULE,
    .open = apds9960_fopen,
    .release = apds9960_fclose,
    .unlocked_ioctl = apds9960_ioctl
};

/**
 * 环境光数据采用 ioctl 进行读取
 */
static void apds9960_als_work(struct work_struct *work) {
    struct apds9960_data *apds9960 = container_of(
        to_delayed_work(work),
        struct apds9960_data,
        als_work);
    
    u8 buf[8];

    if (!apds9960->als_enabled)
        return;

    if (regmap_bulk_read(apds9960->regmap, REG_CDATAL, buf, 8) < 0)
        return;

    apds9960->ad.c = buf[0] | buf[1] << 8;
    apds9960->ad.r = buf[2] | buf[3] << 8;
    apds9960->ad.g = buf[4] | buf[5] << 8;
    apds9960->ad.b = buf[6] | buf[7] << 8;

    if (apds9960->ad.c < ALS_OFF_TH) {
        apds9960->ad.c = 0;
        apds9960->ad.r = 0;
        apds9960->ad.g = 0;
        apds9960->ad.b = 0;

        apds9960->als_enabled = 0;
    }

    atomic_set(&apds9960->ad_ready, 1);
    wake_up_interruptible(&apds9960->wq);

    dev_dbg(&apds9960->client->dev, "[c]%u, [r]%u, [g]%u, [b]%u\r\n",
            apds9960->ad.c, 
            apds9960->ad.r, 
            apds9960->ad.g, 
            apds9960->ad.b);

    schedule_delayed_work(&apds9960->als_work, msecs_to_jiffies(ALS_PROX_SAMPLE_PERIOD));
}

/**
 * 靠近数据采用 ioctl 进行读取
 */
static void apds9960_proximity_work(struct work_struct *work) {
    struct apds9960_data *apds9960 = container_of(
        to_delayed_work(work),
        struct apds9960_data,
        proximity_work);
    
    int proximity;

    if (!apds9960->proximity_enabled)
        return;

    if (regmap_read(apds9960->regmap, REG_PDATA, &proximity) < 0)
        return;

    apds9960->pd.dist = proximity * 100 / 0xff;
    
    if (proximity < PROXIMITY_OFF_TH) {
        apds9960->pd.dist = 0;
        apds9960->proximity_enabled = 0;
    }
        
    atomic_set(&apds9960->pd_ready, 1);
    wake_up_interruptible(&apds9960->wq);

    dev_dbg(&apds9960->client->dev, "[proximity] %u %u%%",
             proximity, apds9960->pd.dist);

    schedule_delayed_work(&apds9960->proximity_work, msecs_to_jiffies(ALS_PROX_SAMPLE_PERIOD));
}

static void apds9960_report_key(struct apds9960_data *apds9960, unsigned int key) {
    input_event(apds9960->input_dev, EV_KEY, key, 1);
    input_sync(apds9960->input_dev);
    input_event(apds9960->input_dev, EV_KEY, key, 0);
    input_sync(apds9960->input_dev);
}

// 计算差距比例
static int calc_ratio(u8 a, u8 b) {
    int sum = a + b;
    if (sum <= 20)
        return 0;
    return (a - b) * 100 / sum;
} 

#define GESTURE_TH  30

/**
 * 手势数据采用 input 子系统进行上报
 */
static void apds9960_gesture_process(struct apds9960_data *apds9960) {
    struct regmap *regmap = apds9960->regmap;
    u8 buf[4];
    int num_datasets, i;
    u16 u = 0, d = 0, l = 0, r = 0;
    int gmode;
    int delta_ud;
    int delta_lr;

    if (regmap_read(apds9960->regmap, REG_GCONF4, &gmode) < 0)
        return;
    gmode = !!(gmode & GMODE);

    if (regmap_read(regmap, REG_GFLVL, &num_datasets) < 0)
        return;
    if (num_datasets == 0) {
        regmap_update_bits(regmap, REG_GCONF4, GFIFO_CLR, GFIFO_CLR);
        return;
    }

    for (i = 0; i < num_datasets; ++i) {
        if (regmap_bulk_read(regmap, REG_GFIFO_U, buf, 4) < 0)
            return;
        u = buf[0];
        d = buf[1];
        l = buf[2];
        r = buf[3];
        // dev_dbg(&apds9960->client->dev, "[up]%u, [down]%u, [left]%u, [right]%u\r\n",
        //      u, d, l, r);
        if (u + d + r + l <= 80)    // 跳过过弱信号
            continue;

        if (!apds9960->gd.ud_in_process) {
            apds9960->gd.first_ud = calc_ratio(u, d);
            apds9960->gd.ud_in_process = 1;
        }
        if (!apds9960->gd.lr_in_process) {
            apds9960->gd.first_lr = calc_ratio(l, r);
            apds9960->gd.lr_in_process = 1;
        }

        apds9960->gd.last_ud = calc_ratio(u, d);
        apds9960->gd.last_lr = calc_ratio(l, r);
    }
    
    if (!(apds9960->gd.ud_in_process || apds9960->gd.lr_in_process))
        return;

    delta_ud = apds9960->gd.last_ud - apds9960->gd.first_ud;
    delta_lr = apds9960->gd.last_lr - apds9960->gd.first_lr;

    // dev_dbg(&apds9960->client->dev, "delta_ud: %d, delta_lr: %d\r\n", delta_ud, delta_lr);

    if (abs(delta_ud) >= GESTURE_TH && apds9960->gd.ud_in_process) {    // 上下
        if (delta_ud > 0) {
            dev_dbg(&apds9960->client->dev, "UP\r\n");
            apds9960_report_key(apds9960, KEY_UP);
        } else {
            dev_dbg(&apds9960->client->dev, "DOWN\r\n");
            apds9960_report_key(apds9960, KEY_DOWN);
        }
        apds9960->gd.ud_in_process = 0;
    } 
    if (abs(delta_lr) >= GESTURE_TH && apds9960->gd.lr_in_process) { // 左右
        if (delta_lr > 0) {
            dev_dbg(&apds9960->client->dev, "LEFT\r\n");
            apds9960_report_key(apds9960, KEY_LEFT);
        } else {
            dev_dbg(&apds9960->client->dev, "RIGHT\r\n");
            apds9960_report_key(apds9960, KEY_RIGHT);
        }
        apds9960->gd.lr_in_process = 0;
    }

    // dev_dbg(&apds9960->client->dev, "[up]%u, [down]%u, [left]%u, [right]%u\r\n",
    //          u, d, l, r);
}

static irqreturn_t apds9960_irq_handler(int irq, void *data) {
    struct apds9960_data *apds9960 = data;

    schedule_work(&apds9960->work);
    return IRQ_HANDLED;
}

static void apds9960_work(struct work_struct *work) {
    struct apds9960_data *apds9960 = container_of(work, struct apds9960_data, work);
    struct regmap *regmap = apds9960->regmap;
    int status, gstatus;

    // 因为ALS、Proximity、Gesture中断共用一条中断线，因此一次中断信号可能包含多个中断
    // Proximity 经常和 Gesture 中断通常同时出现
    // 这里多次尝试是为了在处理一次 INT 低电平期间，新的中断源又出现，而 INT 始终没有重新拉高，因此不会产生新的下降沿
    int retry = 10;
    while (retry--) {
        if (regmap_read(regmap, REG_STATUS, &status) < 0)
            return;
        if (regmap_read(regmap, REG_GSTATUS, &gstatus) < 0)
            return;

        if (status & AINT) {
            if (status & AVALID) {
                apds9960->als_enabled = 1;
                schedule_delayed_work(&apds9960->als_work, 0);
            }
            regmap_write(regmap, REG_CICLEAR, 0);
        }

        if (status & PINT) {
            if (status & PVALID) {
                apds9960->proximity_enabled = 1;
                schedule_delayed_work(&apds9960->proximity_work, 0);
            }
            regmap_write(regmap, REG_PICLEAR, 0);
        }

        if (status & GINT) {
            if (gstatus & GVALID) {
                apds9960_gesture_process(apds9960);
            }
            regmap_update_bits(regmap, REG_GCONF4, GFIFO_CLR, GFIFO_CLR);
        }

        if (regmap_read(regmap, REG_STATUS, &status) < 0)
            return;
        if (regmap_read(regmap, REG_GSTATUS, &gstatus) < 0)
            return;
        if (!(status & (AINT | PINT | GINT)))
            break;
    }
}

static int apds9960_calibration(struct apds9960_data *apds9960) {
    struct regmap *regmap = apds9960->regmap;
    u8 buf[4];
    int i, gstatus;
    u8 max_ud = 0, max_lr = 0;
    u8 ud_done = 0, lr_done = 0;

    // 先关闭所有中断
    regmap_update_bits(regmap, REG_ENABLE, AIEN | PIEN, 0);
    regmap_update_bits(regmap, REG_GCONF4, GIEN, 0);

    regmap_write(regmap, REG_GOFFSET_U, 0);
    regmap_write(regmap, REG_GOFFSET_D, 0);
    regmap_write(regmap, REG_GOFFSET_L, 0);
    regmap_write(regmap, REG_GOFFSET_R, 0);

    dev_dbg(&apds9960->client->dev, "Calibrating, please SLOWLY approach a PLANE object to "\
        "the sensor\r\n");

    // 某个通道临近饱和时获取各通道偏移
    while (!(ud_done && lr_done)) {
        if (regmap_read(regmap, REG_GSTATUS, &gstatus) < 0)
            return -1;
        if (gstatus & GVALID) {
            if (regmap_bulk_read(regmap, REG_GFIFO_U, buf, 4) < 0)
                return -1;

            for (i = 0; i < 2; ++i) {
                if (max_ud < buf[i])
                    max_ud = buf[i];
            }
            for (i = 2; i < 4; ++i) {
                if (max_lr < buf[i])
                    max_lr = buf[i];
            }

            if (!ud_done && max_ud >= 120) {
                regmap_write(regmap, REG_GOFFSET_U, (u8)(buf[0] - min(buf[0], buf[1])));
                regmap_write(regmap, REG_GOFFSET_D, (u8)(buf[1] - min(buf[0], buf[1])));
                ud_done = 1;
            }
            if (!lr_done && max_lr >= 120) {
                regmap_write(regmap, REG_GOFFSET_L, (u8)(buf[2] - min(buf[2], buf[3])));
                regmap_write(regmap, REG_GOFFSET_R, (u8)(buf[3] - min(buf[2], buf[3])));
                lr_done = 1;
            }
        }
    }

    // 恢复中断
    regmap_update_bits(regmap, REG_ENABLE, AIEN | PIEN, 0xff);
    regmap_update_bits(regmap, REG_GCONF4, GIEN, 0xff);
    return 0;
}

static void apds9960_init(struct apds9960_data *apds9960) {
    int val;
    struct regmap *regmap = apds9960->regmap;

    regmap_write(regmap, REG_ENABLE, 0);

    /* 读取设备ID(0xAB) */
    if (regmap_read(regmap, REG_ID, &val) < 0) {
        dev_dbg(&apds9960->client->dev, "Error in reading ID register\r\n");
        return;
    }
    dev_dbg(&apds9960->client->dev, "ID: %#X\r\n", val);

    /* 配置 ALS, Proximity, Gesture */
    regmap_multi_reg_write(regmap, als_threshold_default, ARRAY_SIZE(als_threshold_default));
    regmap_multi_reg_write(regmap, proximity_threshold_default, ARRAY_SIZE(proximity_threshold_default));
    regmap_multi_reg_write(regmap, gesture_threshold_default, ARRAY_SIZE(gesture_threshold_default));
    regmap_update_bits(regmap, REG_CONTROL, AGAIN_MASK | PGAIN_MASK | LDRIVE_MASK, 
        1 | (0 << 2) | (2 << 6));
    regmap_write(regmap, REG_PERS, 0x44);
    regmap_write(regmap, REG_AICLEAR, 0);   // 清除 non-gesture 中断标志

    /* Gesture设置 */
    val = 2 | (1 << 6);
    regmap_write(regmap, REG_GCONF1, val);
    val = GIEN | GFIFO_CLR;
    regmap_write(regmap, REG_GCONF4, val);

    /* 进入手势模式 */
    // apds9960_enter_gmode(apds9960, 0);

    /* 开启 Proximity, Gesture 和 ALS */
    val = PON | PEN | GEN | AEN | AIEN | PIEN;
    regmap_write(regmap, REG_ENABLE, val);

    /* 开启校验 */
    // apds9960_calibration(apds9960);
}

static void apds9960_deinit(struct apds9960_data *apds9960) {
    int val;
    struct regmap *regmap = apds9960->regmap;
    
    val = ~(PON | PEN | GEN | AEN | AIEN | PIEN);
    regmap_write(regmap, REG_ENABLE, val);

    val = ~GIEN & GFIFO_CLR;
    regmap_write(regmap, REG_GCONF4, val);

    // 清除中断标志
    regmap_write(regmap, REG_AICLEAR, 0);
    regmap_update_bits(regmap, REG_GCONF4, GFIFO_CLR, GFIFO_CLR);
}

static int apds9960_probe(struct i2c_client *client, const struct i2c_device_id *ids) {
    int ret;
    struct apds9960_data *apds9960 = kzalloc(sizeof(struct apds9960_data), GFP_KERNEL);
    if (apds9960 == NULL) {
        dev_err(&client->dev, "kmalloc device data error!\r\n");
        return -ENOMEM;
    }
    i2c_set_clientdata(client, apds9960);

    // regmap初始化
    apds9960->regmap = regmap_init_i2c(client, &regmap_cfg);
    if (IS_ERR(apds9960->regmap)) {
        dev_err(&client->dev, "regmap init error!\r\n");
        ret = PTR_ERR(apds9960->regmap);
        goto fail1;
    }

    // 输入设备初始化
    apds9960->input_dev = input_allocate_device();
    if (apds9960->input_dev == NULL) {
        dev_err(&client->dev, "allocate for input device error!\r\n");
        ret = -ENOMEM;
        goto fail2;
    }
    apds9960->input_dev->name = "apds9960";
    ____atomic_set_bit(EV_KEY, apds9960->input_dev->evbit);
    input_set_capability(apds9960->input_dev, EV_KEY, KEY_UP);
    input_set_capability(apds9960->input_dev, EV_KEY, KEY_DOWN);
    input_set_capability(apds9960->input_dev, EV_KEY, KEY_LEFT);
    input_set_capability(apds9960->input_dev, EV_KEY, KEY_RIGHT);
    ret = input_register_device(apds9960->input_dev);
    if (ret < 0) {
        dev_err(&client->dev, "input device register error!\r\n");
        goto fail2;
    }

    // 注册为 misc 设备
    apds9960->misc.minor = MISC_DYNAMIC_MINOR;
    apds9960->misc.name = "apds9960";
    apds9960->misc.nodename = "apds9960";
    apds9960->misc.parent = &client->dev;
    apds9960->misc.fops = &apds9960_fops;
    ret = misc_register(&apds9960->misc);
    if (ret < 0) {
        dev_err(&client->dev, "misc device register error!\r\n");
        goto fail3;
    }

    init_waitqueue_head(&apds9960->wq);
    apds9960->proximity_enabled = 0;
    apds9960->als_enabled = 0;
    atomic_set(&apds9960->ad_ready, 0);
    atomic_set(&apds9960->pd_ready, 0);
    INIT_DELAYED_WORK(&apds9960->proximity_work, apds9960_proximity_work);
    INIT_DELAYED_WORK(&apds9960->als_work, apds9960_als_work);
    apds9960->client = client;

    // 注册中断
    if (client->irq > 0) {
        INIT_WORK(&apds9960->work, apds9960_work);

        ret = request_irq(client->irq, apds9960_irq_handler, 
                          IRQF_TRIGGER_FALLING, 
                          "apds9960-int", apds9960);
        if (ret < 0) {
            dev_err(&client->dev, "irq request error!\r\n");
            goto fail4;
        }
    }

    apds9960_init(apds9960);
    
    return 0;

fail4:
    misc_deregister(&apds9960->misc);
fail3:
    input_unregister_device(apds9960->input_dev);
fail2:
    regmap_exit(apds9960->regmap);
fail1:
    kfree(apds9960);
    return ret;
}

static int apds9960_remove(struct i2c_client * client) {
    struct apds9960_data *apds9960 = i2c_get_clientdata(client);

    apds9960_deinit(apds9960);
    
    if (client->irq > 0) {
        cancel_work_sync(&apds9960->work);
        free_irq(apds9960->client->irq, apds9960);
    }
    
    misc_deregister(&apds9960->misc);
    input_unregister_device(apds9960->input_dev);
    regmap_exit(apds9960->regmap);
    kfree(apds9960);
    return 0;
}

struct of_device_id apds9960_match_table[] = {
    { .compatible = "avago,apds9960" },
    { /* sentinel */ }
};

struct i2c_device_id apds9960_id_table[] = {
    { "apds9960", 0 },
    { /* sentinel */}
};

struct i2c_driver apds9960_driver = {
    .id_table = apds9960_id_table,
    .driver = {
        .of_match_table = of_match_ptr(apds9960_match_table),
        .name = "apds9960",
        .owner = THIS_MODULE
    },
    .probe = apds9960_probe,
    .remove = apds9960_remove
};

module_i2c_driver(apds9960_driver);
MODULE_LICENSE("GPL");