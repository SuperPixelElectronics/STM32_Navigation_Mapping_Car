#include "nRF24L01_API.h"

/*********     24L01发送接收数据宽度定义	  ***********/
#define TX_ADR_WIDTH 5    // 5字节地址宽度
#define RX_ADR_WIDTH 5    // 5字节地址宽度
#define TX_PLOAD_WIDTH 32 // 32字节有效数据宽度
#define RX_PLOAD_WIDTH 32 // 32字节有效数据宽度

const uchar TX_ADDRESS[TX_ADR_WIDTH] = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF}; // 发送地址
const uchar RX_ADDRESS[RX_ADR_WIDTH] = {0xFF, 0xFF, 0xFF, 0xFF,
                                        0xFF}; // 发送地址

void NRF_CSN_reset()  { HAL_GPIO_WritePin(NRF_CSN_GPIO_Port, NRF_CSN_Pin, GPIO_PIN_RESET); }
void NRF_CSN_set()    { HAL_GPIO_WritePin(NRF_CSN_GPIO_Port, NRF_CSN_Pin, GPIO_PIN_SET); }
int NRF_IRQ_read()    { return HAL_GPIO_ReadPin(NRF_IRQ_GPIO_Port, NRF_IRQ_Pin);}

void NRF_SCK_reset()  { HAL_GPIO_WritePin(NRF_SCK_GPIO_Port, NRF_SCK_Pin, GPIO_PIN_RESET); }
void NRF_SCK_set()    { HAL_GPIO_WritePin(NRF_SCK_GPIO_Port, NRF_SCK_Pin, GPIO_PIN_SET); }
void NRF_CE_reset()   { HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, GPIO_PIN_RESET); }
void NRF_CE_set()     { HAL_GPIO_WritePin(NRF_CE_GPIO_Port, NRF_CE_Pin, GPIO_PIN_SET); }
void NRF_MOSI_reset() { HAL_GPIO_WritePin(NRF_MOSI_GPIO_Port, NRF_MOSI_Pin, GPIO_PIN_RESET); }
void NRF_MOSI_set()   { HAL_GPIO_WritePin(NRF_MOSI_GPIO_Port, NRF_MOSI_Pin, GPIO_PIN_SET); }
int NRF_MISO_read()   { return HAL_GPIO_ReadPin(NRF_MISO_GPIO_Port, NRF_MISO_Pin);}

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

  NRF_CE_reset(); // CE拉低，使能24L01配置，此时才可以写TX（TX在配置里面）
  NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf,
                     TX_PLOAD_WIDTH); // 写数据到TX BUF  32个字节
  NRF_CE_set();                       // CE置高，使能发送，拉高为发送
  while (NRF_IRQ_read() == 1)
    ;                                    // 等待发送完成
  state = NRF24L01_Read_Reg(NRF_STATUS); // 读取状态寄存器的值
  NRF24L01_Write_Reg(nRF_WRITE_REG + NRF_STATUS,
                     state); // 清除TX_DS或MAX_RT中断标志，好像不是复位成0
  if (state & MAX_TX)        // 如果达到最大重发次数
  {
    NRF24L01_Write_Reg(FLUSH_TX,
                       0xff); // 清除TX FIFO寄存器，直接清空数据，不要了
    return MAX_TX;
  }
  if (state & TX_OK) // 发送完成
  {
    return TX_OK;
  }
  return 0xff; // 发送失败
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
      0x0f);    // 配置基本工作模式的参数;PWR_UP,EN_CRC,16BIT_CRC,接收模式,开启所有中断

  // --- 开启带载应答功能 ---

  // 1. 发送激活指令（有些芯片需要这个才能写 0x1C 和 0x1D）
  // 如果你的 NRF24L01_Write_Reg 函数只支持 (reg, data)
  // 我们可以直接操作底层的 SPI 写入，或者尝试直接写寄存器
  NRF_CSN_reset();
  SPI_RW(0x50);            // 激活指令
  SPI_RW(0x73);            // 激活数据
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

// private start------------------

static void NRF_Car_Reload_ACK_Payload(void);

// RX for controler Control
static void car_receive_control_combined(uint8_t *rx_buf);

// TX for controler Request
static void NRF_Car_Data_Pack(char *buf);
static void NRF_Send_FullMap_BruteForce(void);
static void NRF_Send_CarTrack_BruteForce(void);

/**************************************************************************
函数功能：小车端：将最新的状态数据“装填”到应答缓冲区
入口参数：无
返回  值：无
**************************************************************************/
static void NRF_Car_Reload_ACK_Payload(void)
{
  NRF_Car_Data_Pack(nrf_send_buf); // 调用你原有的封包函数

  // 清除 TX FIFO，确保装填的是最新数据 (可选)
  NRF24L01_Write_Reg(FLUSH_TX, 0xFF);

  // 使用特殊的指令 0xA8 (W_ACK_PAYLOAD) 将数据写入通道 0 的应答负载
  NRF24L01_Write_Buf(0xA8, (uint8_t *)nrf_send_buf, 32);
}

// RX for controler Control-------------------------------------------------------

/**************************************************************************
函数功能：小车端]手动拆解统一控制包 (增加 Control_Mode 仲裁)
入口参数：无
返回  值：无
**************************************************************************/
static void car_receive_control_combined(uint8_t *rx_buf)
{
  // rx_buf[0] 是 'C'

  // 0. 首先提取控制模式权限位
  nrf_control_mode = (NRFControlModeType)rx_buf[1];

  // --- 关键仲裁逻辑 ---
  // 只有当遥控器明确要求“手动模式”时，小车才会被动接受遥控器的信息
  if (nrf_control_mode == REMOTE_CONTROL)
  {

    // 1. 提取速度 (偏移 2 和 6)
    memcpy(&car_set_velocity.v, &rx_buf[2], 4);
    memcpy(&car_set_velocity.angular, &rx_buf[6], 4);

    // 2. 提取目标点 (偏移 10 和 14)
    memcpy(&car_goal_position.x, &rx_buf[10], 4);
    memcpy(&car_goal_position.y, &rx_buf[14], 4);

    // 3. 提取开关状态 (偏移 18 和 19)
    map_detect_switch = (FunctionalState)rx_buf[18];
    tracker.state = (FunctionalState)rx_buf[19];
  }
  else
  {
    // 如果是 AUTO_MODE (算法模式)，小车直接忽略 rx_buf 里的信息
    // car_set_velocity 会在小车本地的 PID/算法回调中被赋予 Local_Logic_V
  }
}

// TX for controler Request-------------------------------------------------------
/**************************************************************************
函数功能：小车端数据封包发送函数 (严格32字节)
入口参数：无
返回  值：无
**************************************************************************/
static void NRF_Car_Data_Pack(char *buf)
{
  int offset = 0;

  // --- p段: 位置信息 (12字节) ---
  // 使用memcpy直接拷贝内存，避免指针强转带来的对齐问题
  memcpy(&buf[offset], &car_combined_processed.CombinedPosition.x, 4);
  offset += 4;
  memcpy(&buf[offset], &car_combined_processed.CombinedPosition.y, 4);
  offset += 4;
  memcpy(&buf[offset], &car_combined_processed.CombinedPosition.theta, 4);
  offset += 4;

  // --- v段: 实时速度 (8字节) ---
  memcpy(&buf[offset], &car_set_velocity.v, 4);
  offset += 4;
  memcpy(&buf[offset], &car_set_velocity.angular, 4);
  offset += 4;

  // --- g段: 目标位置 (8字节) ---
  memcpy(&buf[offset], &car_goal_position.x, 4);
  offset += 4;
  memcpy(&buf[offset], &car_goal_position.y, 4);
  offset += 4;

  // --- t段: 追踪与状态合并 (4字节) ---

  // 1. 近端索引与预瞄点索引 (强制转为单字节char)
  buf[offset++] = (unsigned char)tracker.near_index;
  buf[offset++] = (unsigned char)tracker.lookahead_index;

  // 2. 状态位合并 (Bitwise Compression)
  // 逻辑：bit0代表路径追踪开关，bit1代表地图探测开关
  unsigned char status_byte = 0;
  if (tracker.state == ENABLE)
    status_byte |= 0x01; // 设置第0位
  if (map_detect_switch == ENABLE)
    status_byte |= 0x02; // 设置第1位
  buf[offset++] = status_byte;

  // 3. 路径总长度 (单字节)
  buf[offset++] = (unsigned char)current_track_length;

  // 此时 offset 应该正好等于 32
}

/**************************************************************************
函数功能：小车暴力发送地图
入口参数：无
返回  值：无
**************************************************************************/
static void NRF_Send_FullMap_BruteForce(void)
{
  // 使用宏计算：200 * 50 = 10000 字节
  const uint16_t TOTAL_MAP_BYTES = CAR_MAP_SIZE * MAP_ROW_BYTES;
  const uint8_t PAYLOAD_PER_PACKET = 30; // 32字节 - 2字节索引

  uint16_t sent_bytes = 0;
  uint16_t packet_index = 0;
  uint8_t *map_ptr = (uint8_t *)car_map;

  // 1. 确保进入发送模式
  NRF_TX_Mode();

  while (sent_bytes < TOTAL_MAP_BYTES)
  {
    // 2. 填充 2 字节序列号 (大端模式)
    nrf_send_buf[0] = (uint8_t)(packet_index >> 8);   // 高8位
    nrf_send_buf[1] = (uint8_t)(packet_index & 0xFF); // 低8位

    // 3. 计算本包长度
    uint16_t remaining = TOTAL_MAP_BYTES - sent_bytes;
    uint8_t current_len = (remaining >= PAYLOAD_PER_PACKET) ? PAYLOAD_PER_PACKET : remaining;

    // 4. 填充地图数据 (从第3个字节开始填)
    memcpy(&nrf_send_buf[2], map_ptr + sent_bytes, current_len);

    // 5. 硬件发送
    NRF24L01_TxPacket((unsigned char *)nrf_send_buf);

    // 6. 核心延时：给接收端留出解析和刷新时间
    // 连发300多包，如果这里不延时，遥控器的接收中断会被淹没
    HAL_Delay(2);

    sent_bytes += current_len;
    packet_index++;
  }

  // 7. 切回接收模式，等待遥控器下一次指令
  NRF_RX_Mode();
}

/**************************************************************************
函数功能：小车暴力发送所有路径点 (约1032字节)
入口参数：无
返回  值：无
**************************************************************************/
static void NRF_Send_CarTrack_BruteForce(void)
{
  const uint16_t TOTAL_TRACK_BYTES = TRACK_LEN_MAX * sizeof(PointType);
  const uint8_t PAYLOAD_PER_PACKET = 30; // 32 - 2字节索引

  uint16_t sent_bytes = 0;
  uint16_t packet_index = 0;
  uint8_t *track_ptr = (uint8_t *)car_track;

  NRF_TX_Mode();

  while (sent_bytes < TOTAL_TRACK_BYTES)
  {
    // 1. 填充2字节序列号
    nrf_send_buf[0] = ((char)packet_index >> 8);
    nrf_send_buf[1] = ((char)packet_index & 0xFF);

    // 2. 计算本包长度
    uint16_t remaining = TOTAL_TRACK_BYTES - sent_bytes;
    uint8_t current_len = (remaining >= PAYLOAD_PER_PACKET) ? PAYLOAD_PER_PACKET : remaining;

    // 3. 拷贝坐标数据
    memcpy(&nrf_send_buf[2], track_ptr + sent_bytes, current_len);

    // 4. 发送
    NRF24L01_TxPacket((uint8_t *)nrf_send_buf);

    // 5. 路径点较少，延迟2ms即可
    HAL_Delay(2);

    sent_bytes += current_len;
    packet_index++;
  }

  NRF_RX_Mode();
}
// private end------------------

/**************************************************************************
函数功能：nrf的处理逻辑，放在中断里面
入口参数：无
返回  值：无
**************************************************************************/
void nrf_receive_process_irq() // 在irq函数里
{
  // if (NRF_IRQ_read() == 0){
  //   NRF24L01_RxPacket((uchar *)nrf_receive_buf); // 将数据放入数组C
  //   NRF_Car_Data_Pack(nrf_send_buf);
  //   NRF_Car_Reload_ACK_Payload();
  // }
}

/**************************************************************************
函数功能：nrf的处理逻辑，放在主函数里面轮询
入口参数：无
返回  值：无
**************************************************************************/
void nrf_receive_process_main()
{                          // 在main函数里
  if (NRF_IRQ_read() == 0) // 如果检测到开始接收数据
  {
    NRF24L01_RxPacket((uchar *)nrf_receive_buf); // 将数据放入数组C

    // RX for controler Request-------------------------------------------------------
    // 根据指令回传特定数据
    if (strcmp(nrf_receive_buf, "info") == 0)
    {
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      NRF_Car_Data_Pack(nrf_send_buf);
      SEND_BUF((uchar *)nrf_send_buf);
      memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    }
    else if (strcmp(nrf_receive_buf, "map") == 0)
    {
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      NRF_Send_FullMap_BruteForce();
      memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    }
    else if (strcmp(nrf_receive_buf, "track") == 0)
    {
      memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
      NRF_Send_CarTrack_BruteForce();
      memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);
    }

    // RX for controler Command-------------------------------------------------------
    // 根据指令执行对应的动作
    if (control_mode == NRF_CTRL)
    {
      if (strcmp(nrf_receive_buf, "solve") == 0) // 需要回传track数据
      {
        memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
        PlanPath_Iterative(car_combined_processed.CombinedPosition, car_goal_position, car_track, &current_track_length);
        Tracker_Init(&tracker, car_track, current_track_length, 0.1f, 0.8f);
        NRF_Send_CarTrack_BruteForce();
        memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);

        map_show_level = 3;
        oled_page = SOLVE_GRAPH_PAGE;
      }
      else if (strcmp(nrf_receive_buf, "process") == 0) // 需要回传地图数据
      {
        memset(nrf_receive_buf, 0, NRF_RECEIVE_CHAR_NUM);
        Map_Global_Opening_InPlace();
        Map_Compress_Process(METER_PIXEL);
        NRF_Send_FullMap_BruteForce();
        memset(nrf_send_buf, 0, NRF_SEND_CHAR_NUM);

        map_show_level = 3;
        oled_page = SOLVE_GRAPH_PAGE;
      }
      // RX for controler Control-------------------------------------------------------
      // 根据指令实时接收遥控器控制信息
      else if (nrf_receive_buf[0] == 'C')
      {
        car_receive_control_combined((uint8_t *)nrf_receive_buf);
      }
    }
  }

  // 装在ACK带载应答信息
  NRF_Car_Data_Pack(nrf_send_buf);
  NRF_Car_Reload_ACK_Payload();
}
