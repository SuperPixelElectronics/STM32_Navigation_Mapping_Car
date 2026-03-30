#include "nRF24L01_API.h"

// private declare start------------------
// base
static void nrf_send_wait(void);

// TX controler Control
static void nrf_send_control_command(void);

// RX for controler Request
static void NRF_Data_Unpack_Optimized(char *buf);
static uint8_t NRF_Receive_MapPacket_Handler(uint8_t *rx_buf);
static uint8_t NRF_Receive_TrackPacket_Handler(uint8_t *rx_buf);
// private declare end------------------

/*********     24L01发送接收数据宽度定义	  ***********/
#define TX_ADR_WIDTH 5    // 5字节地址宽度
#define RX_ADR_WIDTH 5    // 5字节地址宽度
#define TX_PLOAD_WIDTH 32 // 32字节有效数据宽度
#define RX_PLOAD_WIDTH 32 // 32字节有效数据宽度

const uchar TX_ADDRESS[TX_ADR_WIDTH] = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF}; // 发送地址
const uchar RX_ADDRESS[RX_ADR_WIDTH] = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF}; // 发送地址

void NRF_CSN_reset() { HAL_GPIO_WritePin(NRF_CSN_GPIO_Port, NRF_CSN_Pin, GPIO_PIN_RESET); }
void NRF_CSN_set() { HAL_GPIO_WritePin(NRF_CSN_GPIO_Port, NRF_CSN_Pin, GPIO_PIN_SET); }
int NRF_IRQ_read()
{
  return HAL_GPIO_ReadPin(NRF_IRQ_GPIO_Port, NRF_IRQ_Pin);
}

void NRF_SCK_reset() { HAL_GPIO_WritePin(NRF_SCK_GPIO_Port, NRF_SCK_Pin, GPIO_PIN_RESET); }
void NRF_SCK_set() { HAL_GPIO_WritePin(NRF_SCK_GPIO_Port, NRF_SCK_Pin, GPIO_PIN_SET); }
void NRF_CE_reset() { HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, GPIO_PIN_RESET); }
void NRF_CE_set() { HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, GPIO_PIN_SET); }
void NRF_MOSI_reset() { HAL_GPIO_WritePin(NRF_MOSI_GPIO_Port, NRF_MOSI_Pin, GPIO_PIN_RESET); }
void NRF_MOSI_set() { HAL_GPIO_WritePin(NRF_MOSI_GPIO_Port, NRF_MOSI_Pin, GPIO_PIN_SET); }
int NRF_MISO_read()
{
  return HAL_GPIO_ReadPin(NRF_MISO_GPIO_Port, NRF_MISO_Pin);
}

int nrf_init_cnt;
HAL_StatusTypeDef NRF24L01_Init(void)
{
  nrf_error_state = RESET;
  while (NRF24L01_Check())
  {
    nrf_init_cnt++;
    if (nrf_init_cnt > 100)
    {
      nrf_init_cnt = 0;
      nrf_error_state = SET;
      return HAL_TIMEOUT;
    }
  }
  nrf_init_cnt = 0;
  NRF24L01_RT_Init();
  return HAL_OK;
}

void delay_1us(uchar num)
{
  uchar i, j;
  for (i = 0; i < num; i++)
    for (j = 100; j > 0; j--)
      ;
}
void delay_150us(void)
{
  uint i;
  for (i = 0; i < 600; i++)
    ;
}

uchar SPI_RW(uchar byte) // 读写寄存器地址
{
  uchar bit_ctr;
  for (bit_ctr = 0; bit_ctr < 8; bit_ctr++) // 输出8位
  {
    if ((uchar)(byte & 0x80) == 0x80)
      NRF_MOSI_set(); // MSB TO MOSI，高位先行
    else
      NRF_MOSI_reset();
    byte = (byte << 1);      // 高位先行
    NRF_SCK_set();           // 高电平时相互交换数据
    byte |= NRF_MISO_read(); // 记录nrf24l01发送来的字节
    NRF_SCK_reset();         // 拉低时钟
  }
  return byte;
}
/*********************************************/
/* 函数功能：给24L01的寄存器写值（一个字节） */
/* 入口参数：reg   要写的寄存器地址          */
/*           value 给寄存器写的值            */
/* 出口参数：status 状态值                   */
/*********************************************/
uchar NRF24L01_Write_Reg(uchar reg, uchar value)
{
  uchar status;

  NRF_CSN_reset();
  ;                     // CSN=0;
  status = SPI_RW(reg); // 发送寄存器地址,并读取状态值
  SPI_RW(value);        // 先写寄存器，再写值
  NRF_CSN_set();
  ; // CSN=1;

  return status;
}
/*************************************************/
/* 函数功能：读24L01的寄存器值 （一个字节）      */
/* 入口参数：reg  要读的寄存器地址               */
/* 出口参数：value 读出寄存器的值                */
/*************************************************/
uchar NRF24L01_Read_Reg(uchar reg)
{
  uchar value;

  NRF_CSN_reset();
  ;            // CSN=0;
  SPI_RW(reg); // 发送寄存器值(位置),并读取状态值
  value = SPI_RW(NOP);
  NRF_CSN_set();
  ; // CSN=1;

  return value;
}
/*********************************************/
/* 函数功能：读24L01的寄存器值（多个字节）   */
/* 入口参数：reg   寄存器地址                */
/*           *pBuf 读出寄存器值的存放数组    */
/*           len   数组字节长度              */
/* 出口参数：status 状态值                   */
/*********************************************/
uchar NRF24L01_Read_Buf(uchar reg, uchar *pBuf, uchar len)
{
  uchar status, u8_ctr;
  NRF_CSN_reset();
  ;                     // CSN=0
  status = SPI_RW(reg); // 发送寄存器地址,并读取状态值
  for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
    pBuf[u8_ctr] = SPI_RW(0XFF); // 读出数据
  NRF_CSN_set();
  ;              // CSN=1
  return status; // 返回读到的状态值
}
/**********************************************/
/* 函数功能：给24L01的寄存器写值（多个字节）  */
/* 入口参数：reg  要写的寄存器地址            */
/*           *pBuf 值的存放数组               */
/*           len   数组字节长度               */
/**********************************************/
uchar NRF24L01_Write_Buf(uchar reg, uchar *pBuf, uchar len)
{
  uchar status, u8_ctr;
  NRF_CSN_reset();
  ;                     // CSN低电平写
  status = SPI_RW(reg); // 发送寄存器值(位置),并读取状态值
  for (u8_ctr = 0; u8_ctr < len; u8_ctr++)
    SPI_RW(*pBuf++); // 写入数据
  NRF_CSN_set();
  ;
  return status; // 返回读到的状态值
}

/*********************************************/
/* 函数功能：24L01接收数据                   */
/* 入口参数：rxbuf 接收数据数组              */
/* 返回值： 0   成功收到数据                 */
/*          1   没有收到数据                 */
/*********************************************/
uchar NRF24L01_RxPacket(uchar *rxbuf)
{
  uchar state;
  state = NRF24L01_Read_Reg(NRF_STATUS); // 读取状态寄存器的值
  NRF24L01_Write_Reg(
      nRF_WRITE_REG + NRF_STATUS,
      state);        // 0x20+STATUS是指向状态寄存器，state是为了，如果标志位置1那执行该程序就会清除中断
  if (state & RX_OK) // RX_OK是0x40，这里是判断接收
  {
    NRF_CE_reset(); // 好像CE使能配置端，CE为低电平，配置的才有用。而CSN是使能写数据端，低电平才能写数据
    NRF24L01_Read_Buf(
        RD_RX_PLOAD, rxbuf,
        RX_PLOAD_WIDTH);                // 读取数据，好像读数据不用拉高CE,只有发送才要
    NRF24L01_Write_Reg(FLUSH_RX, 0xff); // 清除RX FIFO寄存器
    NRF_CE_set();
    delay_150us();
    return 0;
  }
  return 1; // 没收到任何数据
}
/**********************************************/
/* 函数功能：设置24L01为发送模式              */
/* 入口参数：txbuf  发送数据数组              */
/* 返回值； 0x10    达到最大重发次数，发送失败*/
/*          0x20    成功发送完成              */
/*          0xff    发送失败                  */
/**********************************************/
uchar NRF24L01_TxPacket(uchar *txbuf)
{
  uchar state;
  uint32_t timeout = 0x0FFF; // 增加软超时保护，防止物理损坏导致死机

  NRF_CE_reset();
  NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);
  NRF_CE_set();

  // 1. 增加软超时保护，践行复利：永远不要给系统留下死循环
  while (NRF_IRQ_read() == 1 && timeout > 0)
    timeout--;

  state = NRF24L01_Read_Reg(NRF_STATUS); // 读取状态寄存器

  // 2. 关键：同时清除所有标志位 (TX_DS, MAX_RT, RX_DR)
  // 写入 0x70 可以一次性清除 4, 5, 6 三个标志位
  NRF24L01_Write_Reg(nRF_WRITE_REG + NRF_STATUS, state);

  if (state & MAX_TX)
  {
    NRF24L01_Write_Reg(FLUSH_TX, 0xff);
    return MAX_TX;
  }

  if (state & TX_OK)
  {
    // 3. 核心修正：如果 TX_OK 的同时发现有 RX_OK (0x40)
    // 这说明 ACK Payload 已经跟着应答信号一起回传到了遥控器
    if (state & RX_OK)
    {
      // 立即读取接收到的数据，否则 IRQ 可能无法正常恢复
      NRF24L01_Read_Buf(RD_RX_PLOAD, (unsigned char*)nrf_receive_buf, RX_PLOAD_WIDTH);
      // 这里可以调用你的解析函数
      NRF_Data_Unpack_Optimized(nrf_receive_buf);
    }
    return TX_OK;
  }
  return 0xff;
}

/********************************************/
/* 函数功能：检测24L01是否存在              */
/* 返回值；  0  存在                        */
/*           1  不存在                      */
/********************************************/
uchar NRF24L01_Check(void)
{
  uchar check_in_buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
  uchar check_out_buf[5] = {0x00};

  NRF_SCK_reset();
  NRF_CSN_set();
  ;               // 不写配置
  NRF_CE_reset(); // 使能配置

  NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, check_in_buf, 5); // 写TX

  NRF24L01_Read_Buf(nRF_READ_REG + TX_ADDR, check_out_buf, 5); // 读TX

  if ((check_out_buf[0] == 0x11) && (check_out_buf[1] == 0x22) &&
      (check_out_buf[2] == 0x33) && (check_out_buf[3] == 0x44) &&
      (check_out_buf[4] == 0x55))
    return 0;
  else
    return 1;
}

void NRF24L01_RT_Init(void) // 收发配置初始化
{
  NRF_CE_reset();
  NRF24L01_Write_Reg(nRF_WRITE_REG + RX_PW_P0,
                     RX_PLOAD_WIDTH); // 选择通道0的有效数据宽度
  NRF24L01_Write_Reg(FLUSH_RX, 0xff); // 清除RX FIFO寄存器,0xff是没用的
  NRF24L01_Write_Buf(nRF_WRITE_REG + TX_ADDR, (uchar *)TX_ADDRESS,
                     TX_ADR_WIDTH); // 写TX节点地址
  NRF24L01_Write_Buf(nRF_WRITE_REG + RX_ADDR_P0, (uchar *)RX_ADDRESS,
                     RX_ADR_WIDTH);                    // 设置TX节点地址,主要为了使能ACK
  NRF24L01_Write_Reg(nRF_WRITE_REG + EN_AA, 0x01);     // 使能通道0的自动应答
  NRF24L01_Write_Reg(nRF_WRITE_REG + EN_RXADDR, 0x01); // 使能通道0的接收地址
  NRF24L01_Write_Reg(
      nRF_WRITE_REG + SETUP_RETR,
      0x1a); // 设置自动重发间隔时间:500us + 86us;最大自动重发次数:10次
  NRF24L01_Write_Reg(nRF_WRITE_REG + RF_CH,
                     0); // 设置RF通道为2.400GHz  频率=2.4+0GHz
  NRF24L01_Write_Reg(nRF_WRITE_REG + RF_SETUP,
                     0x0F); // 设置TX发射参数,0db增益,2Mbps,低噪声增益开启
  NRF24L01_Write_Reg(
      nRF_WRITE_REG + CONFIG,
      0x0f); // 配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式,开启所有中断

  // --- 核心修正：开启带载应答功能 ---

  // 1. 发送激活指令（有些芯片需要这个才能写 0x1C 和 0x1D）
  // 如果你的 NRF24L01_Write_Reg 函数只支持 (reg, data)
  // 我们可以直接操作底层的 SPI 写入，或者尝试直接写寄存器
  NRF_CSN_reset();
  SPI_RW(0x50); // 激活指令
  SPI_RW(0x73); // 激活数据
  NRF_CSN_set();

  // 2. 开启所有通道的动态负载支持 (Dynamic Payload)
  NRF24L01_Write_Reg(nRF_WRITE_REG + DYNPD, 0x3F);

  // 3. 开启 FEATURE 寄存器的关键位：
  // bit2: EN_DPL (允许动态长度)
  // bit1: EN_ACK_PAY (允许带载应答)
  NRF24L01_Write_Reg(nRF_WRITE_REG + FEATURE, 0x06);

  NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0f);

  NRF_CE_set();
}
void NRF24L01_RT_receive_ready()
{
  NRF24L01_Write_Reg(FLUSH_RX, 0xff); // 清除RX FIFO寄存器,0xff是没用的
}
void NRF_TX_Mode()
{
  NRF_CE_reset(); // 写寄存器时候要把CE拉低
  NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0e);
  NRF_CE_set();
  delay_1us(15);
}
void NRF_RX_Mode()
{
  NRF_CE_reset();
  NRF24L01_Write_Reg(nRF_WRITE_REG + CONFIG, 0x0f); // 转换为接收模式
  NRF_CE_set();
}
void SEND_BUF(uchar *buf)
{
  NRF_TX_Mode();
  NRF24L01_TxPacket(buf);
  NRF_RX_Mode();
}

// nrf通信调试：
// 核心需求：小车相关参数(如速度等)是需要在遥控器和小车之间共享的，两者都需要可以修改，并且在修改后可以正常同步到另一方。

// 第一个版本：本质上遥控器把小车当成了存储器
//  1.遥控器定时给车子发送“info”请求，小车收到“info”请求后把数据回传给遥控器，遥控器在nrf中断函数内接收并写回遥控器变量
//  2.遥控器由玩家按钮操作触发请求小车复杂数据如地图和路径点的操作，发送map和track请求指令，小车收到后回传相应数据，遥控器在nrf中断函数内接收并写回遥控器变量
//  3.遥控器由玩家按钮操作触发给小车发送控制指令的操作，小车收到后直接写入小车自身变量，不回传数据
// 主要问题：
//  1.遥控器定时给车子发送“info”请求有严格限制，一旦频率过快(<500ms)就会出现遥控器的控制小车指令和遥控器请求小车状态指令冲突。
//    程序永远不知道我什么时候会控制车子，因此在控制车子的那一刻nrf正在请求车子信息的情况还是会发生
//  2.nrf会在进行手柄操控时，小车刚好在完成费时费力的屏幕渲染操作，然后此时刚好也碰到了遥控器发出定时请求小车状态信息，结果这两个指令在小车那边碰到一起了。
//    表现为：如果小车建图功能开启并且oled刷新被打开时，经常oled上的小车数据都变成0了，所有小车信息都无法获取，可是摇杆还可以遥控小车，
//           并且假如小车建图功能关闭或oled刷新关闭，就可以解决这个问题，而复位遥控器也可以解决这这个问题。
//    原因不明，推测还是因为发送包冲突和小车来不及处理这么多数据导致的。毕竟nrf是半双工通信

// 第二个版本： 尝试使用nrf的带载模式
//  1.遥控器定期给小车发控制信息，小车收到后直接写入小车自身变量并发送事先已经填装好的带载模式的ACK数据，遥控器在发送函数里接收带载模式的ACK数据然后存入自己的变量。实现基础数据同步
//  2.遥控器仍然可以发送原本的info map track请求，小车回传数据，遥控器在nrf中断函数内接收并写回遥控器变量
//
//  主要问题1：
//    a.如果我的逻辑是遥控器实时发送自身信息，然后使用小车信息覆盖遥控器信息，会出现按下遥控器摇杆后遥控器信息改变，然后发给小车，但是从小车返回的信息是上一次未推摇杆的信息。
//      在下一次发送时遥控器会发送这个未推开摇杆的信息，同时把正确的已经推开摇杆的信息获取回来，结果就是双方总是一方数据对一方数据错，出现小车的数据一下是推摇杆的数据一下是不推摇杆的数据，出现震荡前进的现象
//    b.如果我的逻辑是遥控器实时发送自身信息，然后使用小车信息存入到遥控器内存的拷贝变量中，虽然此时可以了，但是此时遥控器已经占有主导地位了，小车总是会接收到遥控器自身的信息并覆写自身信息，假如小车此时驾驶算法求解结果出来了，
//      需要用自动驾驶结果来覆写自身速度，在接收到遥控器的信息时会使用遥控器信息又覆写速度。但是此时遥控器发来的信息是遥控器自己的，不是小车发过去后遥控器发回来的信息，也就是说原本的速度同步闭环已经在遥控器的拷贝变量机制那
//      边断开了，同样会有问题。
//  解决办法1：
//    a.在带载模式基础上增加“控制所有权信息 nrf_control_mode”，遥控器会根据玩家是否推动摇杆等操作来给这个位赋值，把这个所有权信息发给小车，小车根据这个信息来仲裁是否根据遥控器信息来覆写自身信息
//      小车和遥控器之间的共享信息包含 car_velocity car_goal_position map_detect_switch tracker.state。
//      经过核查(排除OLED菜单手动修改的情况)，发现会出现持续性的写入冲突位于：
//         小车的car_velocity会受到遥控器和轨迹追踪模块二者控制，因此需要在小车的这两处代码都加上对nrf_control_mode的判断
//         小车的tracker.state会受到遥控器和tracker自身二者控制，因此需要在小车的这两处代码都加上对nrf_control_mode的判断
//      而对于其他二者，即car_goal_position和map_detect_switch，只需要在小车代码的根据nrf信息写入自身变量的代码处加上对nrf_control_mode的判断即可
//
//  主要问题2：
//    a.需要解决小车在定期请求的时候出现的和其他请求相互冲突的情况
//    b.如果遥控器给小车发送了process这类指令，会导致小车在耗时的图像开闭运算算法花费大量时间，无法及时反应遥控器数据，导致遥控器显示的小车数据又全部变成0了，必须复位小车才可以。
//      其实在图像处理算法运行期间，小车不需要运动，不需要回传数据也不需要接收控制。
//  解决办法2：
//    a.在发送原本的info map track请求的时候需要先等待一段时间，直到和上一次发送数据(很可能是遥控器定期发送的控制数据)有一段时间间隔，才会继续运行，发出消息。
//      发送请求后，需要更改遥控器状态机为等待相应数据回传状态，遥控器定期给小车发控制信息前要先查看状态机是否忙碌，如果忙碌就不发送控制信息也不更新自身状态
//      遥控器发现相关数据回传完毕后，再把nef状态机设置为空闲，遥控器继续发送控制数据和接收带载ACK
//    b.遥控器一旦给小车发送process或solve指令的时候，也会把状态机设置为等待地图或等待路径信息，小车在完成图像处理或路径求解后，会发送回地图数据和求解路径结果
//      遥控器发现相关数据回传完毕后，再把nef状态机设置为空闲，遥控器继续发送控制数据和接收带载ACK

// private start------------------

// base-------------------------------------------------------
/**
 * @brief 等待上一个指令发出一段时间后再发送，防止信号冲突
 */
static void nrf_send_wait()
{
  uint32_t now = HAL_GetTick();
  while (now - last_request_time < 200)
  {
    now = HAL_GetTick();
  }
}

// TX controler Control-------------------------------------------------------
/**
 * @brief [遥控器端] 手动拼装发送统一控制包 (增加 Control_Mode)
 */
static void nrf_send_control_command(void)
{
  if (nrf_running_status == NRF_IDLE)
  {
    // 1. 清空缓冲区
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);

    // 2. 依次拼装
    nrf_send_buf[0] = 'C'; // 标识符

    // 新增：控制模式 (Offset 1)
    // 建议逻辑：当摇杆大幅度推动时，代码自动将此值设为 REMOTE_CONTROL
    nrf_send_buf[1] = (uint8_t)nrf_control_mode;

    // 拼装速度 (现在从 Offset 2 开始，原先是 1)
    memcpy(&nrf_send_buf[2], &car_set_velocity.v, 4);
    memcpy(&nrf_send_buf[6], &car_set_velocity.angular, 4);

    // 拼装目标点 (现在从 Offset 10 开始，原先是 9)
    memcpy(&nrf_send_buf[10], &car_goal_position_set.x, 4);
    memcpy(&nrf_send_buf[14], &car_goal_position_set.y, 4);

    // 拼装开关 (现在从 Offset 18 开始，原先是 17)
    nrf_send_buf[18] = (uint8_t)map_detect_switch_set;
    nrf_send_buf[19] = (uint8_t)tracker.state_set;

    // 3. 切换模式并执行发送
    NRF_TX_Mode();
    NRF24L01_TxPacket((unsigned char *)nrf_send_buf);

    last_request_time = HAL_GetTick();
  }
}

// RX for controler Request-------------------------------------------------------
/**
 * @brief  优化后的NRF数据解析函数 (正好32字节)
 * @param  buf: 接收到的 nrf_receive_buf 数组指针
 */
// reveive:
// p <car_pos_x[0]>             <car_pos_x[1]>            <car_pos_x[2]>                  <car_pos_x[3]>
//   <car_pos_y[0]>             <car_pos_y[1]>            <car_pos_y[2]>                  <car_pos_y[3]>
//   <car_pos_theta[0]>         <car_pos_theta[1]>        <car_pos_theta[2]>              <car_pos_theta[3]>
// v <car_velocity_v[0]>        <car_velocity_v[1]>       <car_velocity_v[2]>             <car_velocity_v[3]>
//   <car_velocity_angular[0]>  <car_velocity_angular[1]> <car_velocity_angular[2]>       <car_velocity_angular[3]>
// g <car_goal_x[0]>            <car_goal_x[1]>           <car_goal_x[2]>                 <car_goal_x[3]>
//   <car_goal_y[0]>            <car_goal_y[1]>           <car_goal_y[2]>                 <car_goal_y[3]>
// t <car_near_index>           <car_lookahead_index>     <track_state+map_detect_switch> <car_track_length>

static void NRF_Data_Unpack_Optimized(char *buf)
{
  int offset = 0;

  // --- p: 小车位置信息 (12字节) ---
  memcpy(&car_combined_processed.CombinedPosition.x, &buf[offset], 4);
  offset += 4;
  memcpy(&car_combined_processed.CombinedPosition.y, &buf[offset], 4);
  offset += 4;
  memcpy(&car_combined_processed.CombinedPosition.theta, &buf[offset], 4);
  offset += 4;

  // --- v: 小车实时速度 (8字节) ---
  memcpy(&car_feedback_velocity.v, &buf[offset], 4);
  offset += 4;
  memcpy(&car_feedback_velocity.angular, &buf[offset], 4);
  offset += 4;

  // --- g: 小车目标位置 (8字节) ---
  memcpy(&car_goal_position_feedback.x, &buf[offset], 4);
  offset += 4;
  memcpy(&car_goal_position_feedback.y, &buf[offset], 4);
  offset += 4;

  // --- t: 追踪与状态信息 (4字节) ---

  // 1. 索引信息 (从char转回uint16_t)
  tracker.near_index = (uint16_t)buf[offset++];
  tracker.lookahead_index = (uint16_t)buf[offset++];

  // 2. 状态位拆解 (合并字节处理)
  // 假设发送端：bit0 存 track_state, bit1 存 map_detect_switch
  unsigned char status_byte = buf[offset++];

  // 提取 bit0 为 track_state
  tracker.state_feedback = (status_byte & 0x01) ? ENABLE : DISABLE;

  // 提取 bit1 为 map_detect_switch
  map_detect_switch_feedback = (status_byte & 0x02) ? ENABLE : DISABLE;

  // 3. 路径长度
  current_track_length = (uint16_t)buf[offset++];

  // 至此 offset 正好等于 32，完美！
}

/**
 * @brief 处理地图切片包，并返回是否接收完毕
 * @param rx_buf 当前接收的数据
 * @return 1: 接收完毕, 0: 还在传输中
 */
static uint8_t NRF_Receive_MapPacket_Handler(uint8_t *rx_buf)
{
  const uint8_t PAYLOAD_PER_PACKET = 30;
  const uint16_t TOTAL_MAP_BYTES = CAR_MAP_SIZE * MAP_ROW_BYTES;

  uint16_t index = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];
  uint32_t offset = (uint32_t)index * PAYLOAD_PER_PACKET;

  if (offset < TOTAL_MAP_BYTES)
  {
    uint16_t bytes_to_copy = PAYLOAD_PER_PACKET;
    if (offset + PAYLOAD_PER_PACKET >= TOTAL_MAP_BYTES)
    {
      bytes_to_copy = TOTAL_MAP_BYTES - offset;
      // 拷贝最后一份数据
      memcpy(((uint8_t *)car_map) + offset, &rx_buf[2], bytes_to_copy);
      return 1; // 关键：到达边界，返回完成信号
    }

    memcpy(((uint8_t *)car_map) + offset, &rx_buf[2], bytes_to_copy);
  }
  return 0; // 还没传完
}

/**
 * @brief 处理路径点切片包，并返回是否接收完毕
 * @param rx_buf 当前接收的数据
 *  @return 1: 接收完毕, 0: 还在传输中
 */
static uint8_t NRF_Receive_TrackPacket_Handler(uint8_t *rx_buf)
{
  const uint8_t PAYLOAD_PER_PACKET = 30;
  // 注意：这里要确保两端对 sizeof(PointType) 的理解一致
  const uint16_t TOTAL_TRACK_BYTES = TRACK_LEN_MAX * sizeof(PointType);

  uint16_t index = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];
  uint32_t offset = (uint32_t)index * PAYLOAD_PER_PACKET;

  if (offset < TOTAL_TRACK_BYTES)
  {
    uint16_t bytes_to_copy = PAYLOAD_PER_PACKET;
    if (offset + PAYLOAD_PER_PACKET >= TOTAL_TRACK_BYTES)
    {
      bytes_to_copy = TOTAL_TRACK_BYTES - offset;
      memcpy(((uint8_t *)car_track) + offset, &rx_buf[2], bytes_to_copy);
      return 1; // 关键：路径点传输完毕
    }
    memcpy(((uint8_t *)car_track) + offset, &rx_buf[2], bytes_to_copy);
  }
  return 0;
}
// private end------------------

// TX Request-------------------------------------------------------
/**
 * @brief 发送获取小车信息的请求
 */
void nrf_send_info_request()
{
  if (nrf_running_status == NRF_IDLE)
  {
    nrf_running_status = NRF_WAIT_FOR_PARAM;
    nrf_send_wait();
    sprintf(nrf_send_buf, "info");
    SEND_BUF((unsigned char *)nrf_send_buf);
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    last_request_time = HAL_GetTick();
    // read data in nrf_receive_process_irq()
  }
}

/**
 * @brief 发送获取地图的请求
 */
void nrf_send_map_request()
{
  if (nrf_running_status == NRF_IDLE)
  {
    nrf_running_status = NRF_WAIT_FOR_MAP;
    nrf_send_wait();
    sprintf(nrf_send_buf, "map");
    SEND_BUF((unsigned char *)nrf_send_buf);
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    last_request_time = HAL_GetTick();
    // read data in nrf_receive_process_irq()
  }
}

/**
 * @brief 发送获取路径信息的请求
 */
void nrf_send_track_request()
{
  if (nrf_running_status == NRF_IDLE)
  {
    nrf_running_status = NRF_WAIT_FOR_TRACK; // 需要在 enum 里增加这个状态
    nrf_send_wait();
    sprintf(nrf_send_buf, "track"); // 假设指令为 track
    SEND_BUF((unsigned char *)nrf_send_buf);
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    last_request_time = HAL_GetTick();
    // read data in nrf_receive_process_irq()
  }
}

// TX controler Command-------------------------------------------------------
/**
 * @brief 发送求解路径的指令
 */
void nrf_send_solve_command()
{
  if (nrf_running_status == NRF_IDLE)
  {
    nrf_running_status = NRF_WAIT_FOR_TRACK;
    nrf_send_wait();
    sprintf(nrf_send_buf, "solve");
    SEND_BUF((unsigned char *)nrf_send_buf);
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    last_request_time = HAL_GetTick();
  }
}

/**
 * @brief 发送进行图像处理的指令
 */
void nrf_send_image_process_command()
{
  if (nrf_running_status == NRF_IDLE)
  {
    nrf_running_status = NRF_WAIT_FOR_MAP;
    nrf_send_wait();
    sprintf(nrf_send_buf, "process");
    SEND_BUF((unsigned char *)nrf_send_buf);
    memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    last_request_time = HAL_GetTick();
  }
}

/**
 * @brief 放在nrf中断函数里的消息处理函数
 */
void nrf_receive_process_irq() 
{
  if (NRF_IRQ_read() == 0) // 如果检测到开始接收数据
  {
    if (nrf_running_status == NRF_WAIT_FOR_PARAM)
    {
      NRF24L01_RxPacket((uchar *)nrf_receive_buf); // 将数据放入数组C
      NRF_Data_Unpack_Optimized(nrf_receive_buf);
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      last_packet_time = HAL_GetTick(); // 刷新时间戳
      nrf_running_status = NRF_IDLE;
    }
    else if (nrf_running_status == NRF_WAIT_FOR_MAP)
    {
      NRF24L01_RxPacket((uchar *)nrf_receive_buf); // 将数据放入数组C

      // --- 调用地图专用解析器 ---
      // 调用解析器，并检查是否是最后一包
      if (NRF_Receive_MapPacket_Handler((unsigned char *)nrf_receive_buf) == 1)
      {
        nrf_running_status = NRF_IDLE; // 只有全部地图收完，才释放总线
                                       // 这里可以加一个提示，比如：oled_show_string("Map OK");
        Map_Compress_Process(METER_PIXEL);
      }
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      last_packet_time = HAL_GetTick(); // 刷新时间戳
    }
    else if (nrf_running_status == NRF_WAIT_FOR_TRACK)
    {
      NRF24L01_RxPacket((uchar *)nrf_receive_buf); // 将数据放入数组C

      // --- 调用路径点解析器 ---
      // 调用解析器，并检查是否是最后一包
      if (NRF_Receive_TrackPacket_Handler((uint8_t *)nrf_receive_buf) == 1)
      {
        nrf_running_status = NRF_IDLE; // 只有全部路径收完，才释放总线
      }
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      last_packet_time = HAL_GetTick();
    }
  }
}

/**
 * @brief 放在main函数里里的主函数
 */
void nrf_receive_process_main()
{
  uint32_t now = HAL_GetTick();

  // // 1. 超时处理逻辑
  // if (now - last_packet_time > 150 && now - last_request_time > 150) // 略大于你的轮询间隔
  // {
  //   nrf_running_status = NRF_IDLE;
  // }

  // 2. 限速轮询逻辑
  // 只有在 IDLE 状态，且距离上次请求超过 50ms 时才发起 info 请求
  if ((nrf_running_status == NRF_IDLE) && (now - last_request_time > 50))
  {
    if ((oled_page == CONTROL_PAGE && oled_curser.control_page.selected == ENABLE) ||
        (oled_page == SOLVE_GRAPH_PAGE && oled_curser.solve_graph_page.selected == ENABLE && oled_curser.solve_graph_page.curser == 1))
    {
      nrf_control_mode = REMOTE_CONTROL;
    }
    else
    {
      nrf_control_mode = CAR_CONTROL;
    }
    nrf_send_control_command();
  }
}

