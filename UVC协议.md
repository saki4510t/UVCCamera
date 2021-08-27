## 视频数据格式
下面说明了命令/通知消息布局（在该图中，任意选择消息的控件特定部分的长度（AVHeader + VideoPayload））：

```
  31  30  29  28  27  26  25  24  23  22  21  20  19  18  17  16  15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
|         RES0              | 0 |      CUR       |   SET/NOTIF  |                            EID                                |   DWORD0
|                                OCN=0                          |                     CS=AD_SOURCEDATA/SINKDATA                 |   DWORD1
|                                IPN=0                          |                                ICN=0                          |   DWORD2
|              RES1             |                                            DATALEN                                            |   DWORD3
|                                                           AVHeader                                                            |   DWORD4
|                                                           AVHeader                                                            |   DWORD5
                                                                .
                                                                .
                                                                .
|                                                           AVHeader                                                            |   DWORD11
|                                                         VideoPayload                                                          |   DWORD12
|                                                         VideoPayload                                                          |   DWORD13
                                                                .
                                                                .
                                                                .
|                                                         VideoPayload                                                          |   DWORDn-1
|         Padding Byte          |         Padding Byte          |                           VideoPayload                        |   DWORDn

```

命令或通知消息的内容的结构始终如下：

* 每个DWORD为32位，每个视频数据为多个DWORD，最后一个不足32位在左边补充填充数据
* DWORD0 - DWORD3 固定不变
* DWORD4 - DWORDn 为可变
* 消息的固定部分包含[AVFUNCTION]中定义的必填字段。
* 可变部分包含以下字段：
    * 固定长度的AVHeader，包含有关随后的VideoFrame的相关信息。
    * 可变长度的VideoPayload，包含实际的VideoSample值和一些其他与视频有关的信息。

#### AVHeader
AVHeader正好是32个字节长，并具有以下布局：

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | wFlags | 2 | Bitmap | D0: VideoFrameIDValid <br> D1: HDCPOn <br> D2: ClockInfoValid <br> D15..3: 保留。 应设置为零。
2 | bVideoFrameID | 1 | Number | 视频帧ID
3 | Reserved1 | 1 | Number | 保留。 应设置为零。
4 | dStreamCtr | 4 | Number | HDCP发送器分配的streamCtr值。
8 | qInputCtr | 8 | Number | 与SIP中的第一个完整的16字节加密数据块关联的inputCtr值。
16 | dPTSLow | 4 | Number | PTS的低32位
20 | dESCRBaseLow | 4 | Number | ESCRBase的低32位
24 | wExtensions | 2 Number | D0: PTS的高位 <br> D1: ESCRBase的高位 <br> D10..2: ESCRExt <br> D11..15: 保留。 应设置为零。
26 | Reserved6 | 6 | Number | 保留。 应设置为零。

wFlags字段提供有关AVHeader后续字段的信息，如下所示：

* D0位（VideoFrameIDValid）指示bVideoFrameID字段是否包含有效值（D0 = 0b1）（D0 = 0b0）。允许始终将实现方式将VideoFrameIDValid位设置为0b0，有效地表明它们不支持视频帧ID。
* D1位（HDCPOn）指示当前的视频流是否受保护并且是否经过HDCP加密（D1 = 0b1）（D1 = 0b0）。
* D2位（ClockInfoValid）指示dPTSLow，dESCRBaseLow和wExtensions字段是否包含有效值（D2 = 0b1）或不包含（D2 = 0b0）。
* D15..3位保留，应设置为零。

bVideoFrameID字段包含当前VideoFrame的视频帧ID。 VideoFrame ID是一个计数器值，对于在源处生成的每个新VideoFrame，该计数器值都将增加一个，并在达到值255（0xFF）时翻转为零（0x00）。如果wFlags字段中的VideoFrameIDValid位未设置，则发送器应将bVideoFrameID字段设置为零，而接收器应忽略bVideoFrameID字段。

Reserved1字段是保留字段，应设置为零。

HDCP保护机制使用dStreamCtr和qInputCtr字段。每当传输受保护的内容时，应在每个包含受保护的VideoFrame的CBP消息的AVHeader中将wFlags字段中的HDCPOn位置1。如果未设置wFlags字段中的HDCPOn位，则与HDCP相关的字段无关，发送器应将其设置为零，而接收器应将其忽略。

* dStreamCtr字段包含与HDCP发射机分配的视频流相关的streamCtr值。

* qInputCtr字段包含与VideoFrame中的第一个16字节加密数据块关联的inputCtr值。

当设置了wFlags字段中的ClockInfoValid位时，与时钟信息相关的字段（dPTSLow，dESCRBaseLow和wExtensions字段）应具有有效的信息。 如果未设置wFlags字段中的ClockInfoValid位，则与时钟信息相关的字段将不相关，并且发送方应将它们设置为零，而接收方应忽略它们。

* PTS是一个33位的值，其32个低位在dPTSLow字段中编码，而高位在wExtensions字段的Bit D0中编码。 PTS包含在[IEC13818_1]第2.4.3.7节，公式2-11中定义的图像时间戳：
```
PTS(i) = ((SystemClockFrequency * t_F(i))DIV 300)MOD 2^33
```

其中，系统时钟频率（SystemClockFrequency）是27MHz系统时钟，而t_F（i）是视频帧（i）的显示时间。

* ESCR由两部分组成，如[IEC13818_1]，2.4.3.7节，公式2-13、2-14和2-15中所定义的ESCRBase和ESCRExt：
```
ESCR(i) = ESCRBase(i) * 300 + ESCRExt(i)

ESCRBase(i) = ((SystemClockFrequency * t(i))DIV 300)MOD 2^33
ESCRExt(i) = ((SystemClockFrequency * t(i))DIV 1)MOD 300
```

其中系统时钟频率（SystemClockFrequency）是27MHz系统时钟，而t（i）是视频帧（i）中第一个字节的预期到达时间。

注意：当使用这些字段来传达时序信息时，假定使用了[IEC13818_1]中定义的同步模型。 这意味着dPTSLow，dESCRBaseLow和wExtensions字段的更新速率应大于或等于[IEC13818_1]要求的更新速率。 在没有VideoPayload数据的情况下（使用未压缩的部分帧方法时），仅应发送AVHeader。

Reserved6字段是保留字段，应设置为零。

#### VideoPayload
VideoPayload数据的组织取决于VideoPayload是未压缩还是已压缩。在所有情况下，VideoPayload均应对齐16字节。 这是通过将实际VideoPayload零填充到16字节的最接近倍数来实现的。

###### 未压缩的视频
下图描述了与SourceData或SinkData AVControl交换未压缩的全帧或部分帧VideoPayload时使用的VideoPayload布局。 （为了完整起见，AVHeader包含在图中，但不属于VideoPayload。）

```
 |    AVHeader    |
 | InfoBlock Hdr  |
 | InfoBlock Data |
 | InfoBlock Hdr  |
 | InfoBlock Data |
 | InfoBlock Hdr  |
 | InfoBlock Data |
         .
         .
         .
 | InfoBlock Hdr  |
 | InfoBlock Data |
```

注意1：仅在更新一个以上子区域时存在。 有关详情，请参见下文。

注意2：VideoPayload的末尾可能会出现一些其他的零填充字节（此处未显示）。

VideoPayload始终包含与单个VideoFrame相关的视频信息。 所有视频信息都组织成一个或多个InfoBlock数据结构，如下节所述。

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | X | 2 | Number | 子区域的X坐标.
2 | Y | 2 | Number | 子区域的Y坐标.
4 | W | 2 | Number | 子区域的宽.
6 | H | 2 | Number | 子区域的高.
8 | VideoDataLength | 4 | Number | 子区域的VideoPayload的字节数：p.
12 | VideoData | p | - | 该子区域的实际VideoPayload.

VideoData字段中的VideoPayload由位于VideoFrame的连续矩形子区域中的一组VideoSamples组成，由X，Y，W和H字段标识。  VideoPayload中的字节数在VideoDataLength字段中指示。

由于存在要更新的各个子区域，因此存在的InfoBlock数据结构数量就很多。

当使用未压缩全帧方法时，应仅存在一个InfoBlock，X和Y字段均应设置为零，W和H字段应分别设置为图像宽和高。

使用未压缩的部分帧方法时，可能存在一个或多个InfoBlock。 部分更新定义阶段（请参见图“ AVFormat 1数据流”）负责创建子区域，方法是确定哪些VideoSample在当前VideoFrame和以前的VideoFrame之间已更改，并将它们分组为一组平衡的矩形区域， 最小化那些矩形区域中包含的冗余（不变）VideoSamples的数量，但同时也将矩形区域的数量保持为最小。 实现此目标的确切算法不在本规范范围内。

InfoBlock结构在VideoPayload中出现的顺序很重要。 更新应以确切的顺序应用，以便获得确定的结果。

## 传输定界符
同步数据流本质上是连续的，尽管每个数据包发送的实际字节数可能会在流的整个生命周期中发生变化。 为了指示等时数据流中的临时停止而不关闭管道（从而放弃USB带宽），需要定义一个带内传输定界符。 本规范将两种情况视为传输定界符。 第一个是零长度数据包，第二个是通常通常具有同步传输的ServiceInterval（请参见下文）中没有同步传输。 两种情况都被认为是等效的，并且AVFunction的行为应相同。 在这两种情况下，本规范都将传输定界符视为可以通过USB发出信号的实体。

#### 服务间隔和服务间隔包定义
为了更好地描述视频的打包，引入了ServiceInterval（SI）的概念。 ServiceInterval表示在其中同步端点服务一次的USB（微）帧或总线间隔的数量。对于全/高速同步端点，ServiceInterval定义为：
```
SI = (micro)frame * 2^(bIntrrval - 1)
```

有关bInterval字段，其允许值及其使用的更多信息，请参见[USB2.0]规范。对于SuperSpeed等时端点，ServiceInterval与[USB3.0]规范中定义的Service Interval相同：
```
SI = Service Interval = Bus Interval * 2^(bIntrrval - 1)
```

另外，引入了ServiceIntervalPacket（SIP）。 ServiceIntervalPacket定义为一个包，其中包含在ServiceInterval期间通过总线传输的所有样本。 对于全/高速端点，SIP与通过USB传输的物理数据包完全相同。 对于高速高带宽端点，SIP是在微帧中通过总线传输的两个或三个物理数据包的串联。 对于SuperSpeed端点，SIP是在ServiceInterval中通过总线传输的最多48个物理数据包（3个突发机会，最多16个突发）的串联。上面的定义提供了“每个ServiceInterval一个ServiceInterval数据包（SIP）”的模型，而与USB上的实际事务无关。

## 视频传输
通过USB传输时，未压缩的VideoSamples首先被分段为VideoParticle，然后打包为整数个字节，称为VideoSubSlot。 然后将多个VideoSlot打包到SIP中。以下各节提供了更多详细信息。

## 有效载荷标题的格式（有效载荷标题的格式）
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bHeaderLength | 1 | Number | 有效负载头的长度（以字节为单位），包括此字段。
1 | bmHeaderInfo | 1 | Bitmap | 提供有关标题后面的示例数据的信息，以及此标题中可选标题字段的可用性。 <br> D0: Frame ID – 对于基于帧的格式，每当新的视频帧开始时，该位在0和1之间切换。 对于基于流的格式，在每个新的特定于编解码器的段的开头，该位在0和1之间切换。 此行为对于基于帧的有效载荷格式（例如DV）是必需的，对于基于流的有效载荷格式（例如MPEG-2 TS）是可选的。 对于基于流的格式，必须通过“视频探测和提交”控件的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。 <br> D1: End of Frame – 如果以下有效负载数据标记了当前视频或静止图像帧的结束（对于基于帧的格式），或者指示编解码器特定的段的结束（对于基于流的格式），则该位置1。 对于所有有效负载格式，此行为都是可选的。 对于基于流的格式，必须通过“视频探测和提交控件”的bmFramingInfo字段指示对此位的支持（请参见第4.3.1.1节“视频探测和提交控件”）。 <br> D2: Presentation Time – 如果dwPresentationTime字段作为标头的一部分发送，则此位置1。 <br> D3: Source Clock Reference – 如果dwSourceClock字段作为标头的一部分发送，则此位置1。 <br> D4: 有效负载特定位。 请参阅各个有效负载规格以进行使用。 <br> D5: Still Image – 如果随后的数据是静止图像帧的一部分，则设置此位，并且仅用于静止图像捕获的方法2和3。 对于时间编码格式，该位指示随后的数据是帧内编码帧的一部分。 <br> D6: Error – 如果此有效载荷的视频或静止图像传输出错，则设置此位。 流错误代码控件将反映错误原因。 <br> D7: End of header – 如果这是数据包中的最后一个报头组，则设置此位，其中报头组引用此字段以及由该字段中的位标识的任何可选字段（为将来的扩展定义）。

## 流头
以下是对未压缩流的包头格式的描述。长度48位

```
HLE     |                  Header Length                |
--------------------------------------------------------
BFH[0]  | EOH | ERR | STI | RES | SCR | PTS | EOF | FID |
--------------------------------------------------------
PTS     |                   PTS [7:0]                   |
        |                   PTS [15:8]                  |
        |                   PTS [23:16]                 |
        |                   PTS [31:24]                 |
--------------------------------------------------------
SCR     |                   SCR [7:0]                   |
        |                   SCR [15:8]                  |
        |                   SCR [23:16]                 |
        |                   SCR [31:24]                 |
        |                   SCR [39:32]                 |
        |                   SCR [47:40]                 |
```

* HLE，标头长度字段，标头长度字段指定标头的长度（以字节为单位）。
* BFH，位字段头字段
    * FID: 帧标识符，该位在每个帧起始边界处切换，并在其余帧中保持不变。
    * EOF: 帧结束，该位指示视频帧的结束，并在属于帧的最后一个视频样本中设置。
    * PTS: Presentation Time Stamp，该位置1时表示存在PTS字段。
    * SCR: 源时钟参考，该位置1时表示存在SCR字段。
    * RES: 保留，设置为0。
    * STI: 静止图像，置位时，将视频样本标识为属于静止图像。
    * ERR: 错误位，该位置1时，表明设备流中存在错误。
    * EOH: 标头结尾，该位置1时，指示BFH字段的结尾。
* PTS，图像时间戳记（PTS）字段。当BFH[0]字段中的PTS位置1时，将显示PTS字段。 请参见“视频设备的USB设备类别定义”规范中的第2.4.3.3节“视频和静止图像有效载荷标题”。
* SCR，源时钟参考（SCR）字段。当在BFH [0]字段中设置SCR位时，将出现SCR字段。 请参见“视频设备的USB设备类别定义”规范中的第2.4.3.3节“视频和静止图像有效载荷标题”。

#### PTS
图像时间戳（PTS）。

开始原始帧捕获时，以本机设备时钟为单位的源时钟时间。 对于包括单个视频帧的多个有效载荷传输，可以重复此字段，但要限制该值在整个视频帧中保持相同。PTS与视频探针控制响应的dwClockFrequency字段中指定的单位相同。

#### SCR
由两部分组成的源时钟参考（SCR）值

* D31..D0：本机设备时钟单元中的源时间时钟
* D42..D32：1KHz SOF令牌计数器
* D47..D43：保留，设置为零。

最低有效的32位（D31..D0）包含从源处的系统时间时钟（STC）采样的时钟值。时钟分辨率应根据本规范表4-47中定义的设备的探测和提交响应的dwClockFrequency字段。该值应符合相关的流有效载荷规范。采样STC的时间必须与USB总线时钟相关联。

为此，SCR的下一个最高11位（D42..D32）包含一个1 KHz SOF计数器，表示在对STC进行采样时的帧号。在任意SOF边界对STC进行采样。SOF计数器的大小和频率与与USB SOF令牌关联的帧号相同。但是，不需要匹配当前的帧号。这允许使用可以触发SOF令牌（但无法准确获取帧号）的芯片组来实现，以保留其自己的帧计数器。

保留最高有效的5位（D47..D43），并且必须将其设置为零。

包含SCR值的有效负载报头之间的最大间隔为100ms或视频帧间隔，以较大者为准。 允许间隔更短。

## 有效载荷特定信息
对于未压缩的视频格式，必须使用颜色匹配描述符。 有关详细信息，请参阅视频设备的通用串行总线设备类定义中的“颜色匹配描述符”部分。

#### 描述符
本节提供有关以下描述符的详细信息：

* 未压缩的视频格式描述符
* 未压缩的帧描述符

###### 未压缩的视频格式描述符
未压缩的视频格式描述符定义了特定视频流的特征。 它用于承载未压缩视频信息的格式，包括所有YUV变体。对应于USB IN或OUT端点的终端及其所属的接口支持一种或多种格式定义。 为了选择特定格式，主机软件将控制请求发送到相应的接口。

bFormatIndex字段包含此格式描述符的从一开始的索引，并且主机发出的请求用于设置和获取当前视频格式。

guidFormat字段唯一标识在与此接口进行相应格式索引通信时应使用的视频数据格式。 对于视频源功能，主机软件将根据此字段中指定的格式部署相应的视频格式解码器（如果需要）。

bAspectRatioX和bAspectRatioY字段分别为视频场（隔行）数据指定图片高宽比的X和Y尺寸。 例如，对于16：9的显示，bAspectRatioX将为16，bAspectRatioY将为9。

未压缩的视频格式描述符后跟一个或多个未压缩的视频帧描述符； 每个视频帧描述符都传达特定于该格式支持的帧大小的信息。未压缩的视频格式描述符标识以下内容。

未压缩的视频格式描述符

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | Number | 该描述符的大小（以字节为单位）：27
1 | bDescriptorType | 1 | Constant | CS_INTERFACE描述符类型
2 | bDescriptorSubtype | 1 | Constant | VS_FORMAT_UNCOMPRESSED描述符子类型
3 | bFormatIndex | 1 | Number | 该格式描述符的索引
4 | bNumFrameDescriptors | 1 | Number | 遵循此格式的帧描述符的数量
5 | guidFormat | 16 | GUID | 全局唯一标识符，用于识别流编码格式
21 | bBitsPerPixel | 1 | Number | 每个像素用于在解码视频帧中指定颜色的位数
22 | bDefaultFrameIndex | 1 | Number | 此流的最佳帧索引（用于选择分辨率）
23 | bAspectRatioX | 1 | Number | 图片长宽比的X维度。
24 | bAspectRatioY | 1 | Number | 图片长宽比的Y维度。
25 | bmInterlaceFlags | 1 | Bitmap | 指定隔行信息。 如果此流支持“摄像机终端”中的扫描模式控件，则此字段应反映隔行模式中使用的字段格式（PAL中的顶部字段是字段1，NTSC中的顶部字段是字段2。）： <br> D0: 隔行扫描流或变量。 1 = 是 <br> D1: 每帧的字段。 0= 字段2, 1 = 字段1 <br> D2: 第一个字段。 1 = Yes <br> D3: 已预留 <br> D5..4: 场模式 <br> 00 = 仅字段1 <br> 01 = 仅字段2 <br> 10 = 字段1和2的规则模式 <br> 11 = 字段1和2的随机模式 <br> D7..6: 保留。 不使用。
26 | bCopyProtect | 1 | Boolean | 指定是否限制视频流的复制： <br> 0: 无限制 <br> 1: 限制重复

###### 未压缩的帧描述符
未压缩的视频帧描述符（或简称为帧描述符）用于描述解码视频和静止图像的帧尺寸以及特定流支持的其他特定于帧的特性。 一个或多个帧描述符遵循它们所对应的未压缩视频格式描述符。 帧描述符还用于确定指定帧大小支持的帧间隔范围。

未压缩视频帧描述符仅适用于适用未压缩视频格式描述符的视频格式（请参见第3.1.1节“未压缩视频格式描述符”）。

bFrameIndex字段包含此帧描述符的从一开始的索引，并且主机请求使用它来设置和获取所使用格式的当前帧索引。 对于设备支持的每个相应格式描述符，此索引均基于一个索引。

支持的帧间隔范围可以是连续范围或一组离散值。 对于连续范围，dwMinFrameInterval，dwMaxFrameInterval和dwFrameIntervalStep指示范围的限制和粒度。 对于离散值，dwFrameInterval（x）字段指示在此帧大小下支持的帧间隔范围（以及帧速率）。 帧间隔是以100ns为单位的单个解码视频帧的平均显示时间。

帧描述符标识以下内容。

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | Number | 当bFrameIntervalType为0时，此描述符的大小以字节为单位；当bFrameIntervalType> 0时，此描述符的大小以字节为单位：26+（4 * n）
1 | bDescriptorType | 1 | Constant | CS_INTERFACE描述符类型
2 | bDescriptorSubtype | 1 | Constant | VS_FRAME_UNCOMPRESSED描述符子类型
3 | bFrameIndex | 1 | Number | 该帧描述符的索引
4 | bmCapabilities | 1 | Number | D0: 支持静止图像指定此帧设置是否支持静止图像。 这仅适用于具有使用静止图像捕获方法1的IN视频终结点的VS接口，在所有其他情况下应将其设置为0。 <br> D1: 固定帧速率指定设备是否在与此帧描述符关联的流上提供固定帧速率。如果启用了固定速率，则设置为1；否则，设置为0。 <br> D7..2: 保留，设置为0。
5 | wWidth | 2 | Number | 解码后的位图帧的宽度（以像素为单位）
7 | wHeight | 2 | Number | 解码后的位图帧的高度（以像素为单位）
9 | dwMinBitRate | 4 | Number | 指定最长帧间隔处的最小比特率，以bps为单位可以传输数据。
13 | dwMaxBitRate | 4 | Number | 指定可以传输数据的最短帧间隔的最大比特率，以bps为单位。
17 | dwMaxVideoFrameBufferSize | 4 | Number | 不建议使用此字段。指定压缩器将为视频帧或静止图像生成的最大字节数。Video Probe and Commit控件的dwMaxVideoFrameSize字段替换了此描述符字段。 为了与实现本规范较早版本的主机软件兼容，应选择该字段的值。
21 | dwDefaultFrameInterval | 4 | Number | 指定设备要指示用作默认值的帧间隔。 这必须是以下字段中所述的有效帧间隔。
25 | bFrameIntervalType | 1 | 编号指示如何设置帧间隔： <br> 0: 连续帧间隔 <br> 1..255: 支持的离散帧间隔数（n）
26…  |  |  | 请参阅以下帧间隔表。

连续帧间隔

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
26 | dwMinFrameInterval | 4 | Number | 支持的最短帧间隔（以最高帧速率），以100 ns为单位。
30 | dwMaxFrameInterval | 4 | Number | 支持的最长帧间隔（最低帧速率），以100 ns为单位。
34 | dwFrameIntervalStep | 4 | Number | 表示帧间隔范围的粒度，以100 ns为单位。

离散帧间隔

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
26 | dwFrameInterval(1) | 4 | Number | 支持的最短帧间隔（以最高帧速率），以100 ns为单位。
… | … | … | … | …
26+(4*n)-4 | dwFrameInterval(n) | 4 | Number | 支持的最长帧间隔（最低帧速率），以100 ns为单位。

#### 视频样本
每个未压缩的帧均被视为单个视频样本。 视频样本由一个或多个有效载荷传输（如视频设备的USB设备类别规范中定义）组成。

对于同步管道，每个（微）帧将包含单个有效载荷传输。 每次有效负载传输将由一个有效负载报头组成，紧随其后是一个或多个数据事务中的有效负载数据（对于高速高带宽端点，最多为3个数据事务）。

对于散装管道，每个有效载荷传输的第一个散装数据包应在包的开头包含一个有效载荷报头，然后是有效载荷数据，并根据需要扩展到其他散装数据事务。

#### 未压缩的有效载荷信息
以下段落描述了有效负载传输的约束。

###### 平面格式
平面有效载荷的传输没有数据对齐限制。

###### 打包格式
打包的有效负载格式的传输必须在宏像素边界上对齐。

#### 基于流的格式描述符
UVC 1.1以上版本支持

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | Number | 此描述符的大小，以字节为单位：24
1 | bDescriptorType | 1 | Constant | CS_INTERFACE描述符类型
2 | bDescriptorSubtype | 1 Constant | VS_FORMAT_STREAM_BASED描述符子类型
3 | bFormatIndex | 1 Number | 该格式描述符的索引
4 | guidFormat | 16 | GUID | 全局唯一标识符，用于识别流编码格式
20 | dwPacketLength | 4 | Number | 如果非零，则表示面向数据包的流中格式特定的数据包大小。 如果为零，则表示格式特定的数据是面向字节的或由可变大小格式特定的数据包组成。 有关更多详细信息，请参见本规范的第2.2节“有效载荷数据”。

#### Motion-JPEG视频格式描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | Number | 此描述符的大小，以字节为单位：11
1 | bDescriptorType | 1 | Constant | CS_INTERFACE描述符类型。
2 | bDescriptorSubtype | 1 | Constant | VS_FORMAT_MJPEG描述符子类型
3 | bFormatIndex | 1 | Number | 该格式描述符的索引
4 | bNumFrameDescriptors | 1 | Number | 与此格式相对应的后续帧描述符的数量
5 | bmFlags | 1 | Number | 指定此格式的特征 <br> D0: FixedSizeSamples. 1 = 是 <br> 所有其他位保留供将来使用，应将其重置为零。
6 | bDefaultFrameIndex | 1 | Number | 此流的最佳帧索引（用于选择分辨率）
7 | bAspectRatioX | 1 | Number | 图片长宽比的X维度。
8 | bAspectRatioY | 1 | Number | 图片长宽比的Y维度。
9 | bmInterlaceFlags | 1 | Bitmap | 指定隔行信息。 如果此流支持“摄像机终端”中的扫描模式控件，则此字段应反映隔行模式中使用的字段格式。（PAL中的顶部字段是字段1，NTSC中的顶部字段是字段2。）： <br> D0: 隔行扫描流或变量。 1 =是 <br> D1: 每帧的字段。 0= 字段2, 1 = 字段1 <br> D2: 字段1优先. 1 = 是 <br> D3: 已预留 <br> D5..4: 字段模式 <br> 00 = 仅字段1 <br> 01 = 仅字段2 <br> 10 = 字段1和2的规则模式 <br> 11 = 字段1和2的随机模式 <br> D7..6: 保留。 不使用。
10 | bCopyProtect | 1 B| oolean | 指定是否应限制视频流的复制： <br> 0: 无限制 <br> 1: 限制重复

###### MJPEG视频帧描述符
MJPEG视频帧描述符（或简称为帧描述符）用于描述解码的视频和静止图像的帧尺寸以及特定流支持的其他特定于帧的特性。 一个或多个帧描述符遵循它们所对应的MJPEG视频格式描述符。 帧描述符还用于确定指定帧大小支持的帧间隔范围。

MJPEG视频帧描述符仅适用于适用MJPEG视频格式描述符的视频格式（请参阅第3.1.1节“ MJPEG视频格式描述符”）。

bFrameIndex字段包含此帧描述符的从一开始的索引，并且主机请求使用它来设置和获取所使用格式的当前帧索引。 对于设备支持的每个相应的格式描述符，此索引都是基于一个的。

支持的帧间隔范围可以是连续范围或一组离散值。 对于连续范围，dwMinFrameInterval，dwMaxFrameInterval和dwFrameIntervalStep指示范围的限制和粒度。 对于离散值，dwFrameInterval（x）字段指示在此帧大小下支持的帧间隔范围（以及帧速率）。 帧间隔是以100ns为单位的单个解码视频帧的平均显示时间。

帧描述符标识以下内容。

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | Number | 当bFrameIntervalType为0时，此描述符的大小以字节为单位。当bFrameIntervalType> 0时，此描述符的大小以字节为单位：26+（4 * n）
1 | bDescriptorType | 1 | Constant |CS_INTERFACE描述符类型
2 | bDescriptorSubtype | 1 Constant | VS_FRAME_MJPEG描述符子类型
3 | bFrameIndex | 1 | Number | 该帧描述符的索引
4 | bmCapabilities | 1 | Number | D0: 支持静止图像指定此帧设置是否支持静止图像。 这仅适用于具有使用静止图像捕获方法1的IN视频终结点的VS接口，在所有其他情况下应将其设置为0。 <br> D1: 固定帧速率指定设备是否在与此帧描述符关联的流上提供固定帧速率。 如果启用了固定速率，则设置为1；否则，设置为1。 否则，设置为0。 <br> D7..2: 保留，设置为0。
5 | wWidth | 2 | Number | 解码后的位图帧的宽度（以像素为单位）
7 | wHeight | 2 | Number | 解码后的位图帧的高度（以像素为单位）
9 | dwMinBitRate | 4 | Number | 指定默认压缩质量下的最小比特率和最长帧间隔（以bps为单位），可以以该速率传输数据。
13 | dwMaxBitRate | 4 | Number | 指定默认压缩质量下的最大比特率和最短帧间隔（以bps为单位），可以以该速率传输数据。
17 | dwMaxVideoFrameBufferSize | 4 | Number | 不建议使用此字段。 指定压缩器将产生的视频（或静止图像）帧的最大字节数。“视频探测和提交”控件的dwMaxVideoFrameSize字段替换此描述符字段。 为了与实现本规范较早版本的主机软件兼容，应选择该字段的值。
21 | dwDefaultFrameInterval | 4 | Number | 指定设备要指示用作默认值的帧间隔。 这必须是以下字段中所述的有效帧间隔。
25 | bFrameIntervalType | 1 | Number | 指示如何设置帧间隔：0：连续帧间隔1..255：支持的离散帧间隔数（n）
26… |  |  |  | 请参阅以下帧间隔表。

连续帧间隔

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
26 | dwMinFrameInterval | 4 | Number | 支持的最短帧间隔（以最高帧速率），以100ns为单位。
30 | dwMaxFrameInterval | 4 | Number | 支持的最长帧间隔（最低帧速率），以100ns为单位。
34 | dwFrameIntervalStep | 4 | Number | 表示帧间隔范围的粒度，以100ns为单位。

离散帧间隔

Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
26 | dwFrameInterval(1) | 4 | Number | 支持的最短帧间隔（以最高帧速率），以100ns为单位。
… | … | … | … | …
26+(4*n)-4 | dwFrameInterval(n) | 4 | Number | 支持的最长帧间隔（最低帧速率），以100ns为单位。

###### 视频样本
每个MJPEG帧均被视为单个视频样本。 视频样本由一个或多个有效载荷传输（如视频设备的USB设备类别规范中定义）组成。

对于同步管道，每个（微）帧将包含单个有效载荷传输。 每次有效负载传输将由一个有效负载报头组成，紧随其后是一个或多个数据事务中的有效负载数据（对于高速高带宽端点，最多为3个数据事务）。

对于散装管道，每个有效载荷传输的第一个散装数据包应在包的开头包含一个有效载荷报头，然后是有效载荷数据，并根据需要扩展到其他散装数据事务。

###### MJPEG有效载荷信息
MJPEG有效负载的每一帧都通过JPEG压缩进行编码，并在其头之前包含一个标题，该标题包含诸如压缩表和霍夫曼编码表之类的压缩参数的必需和可选定义。 必需和可选参数用“标记”标识，并包含标记段。

每个帧的结构如下。

* SOI (图像开始, 0xFFD8) – 必要
* APPn (应用标记, 0xFFEn) – 可选，除非使用隔行视频，在这种情况下，需要带有“ AVI1”标记和字段ID信息的APP0段。
* DRI (定义重启间隔, 0xFFDD) – 可选
* DQT (定义量化表, 0xFFDB) – 必要
* DHT (定义霍夫曼表, 0xFFC4) – 可选, 如果未指定，则使用JPEG标准（ISO 10918-1）第K.3.3节中指定的标准表。
* SOF0 (帧开始, 0xFFC0)- 必要. 不支持所有其他SOFn段。
* SOS (扫描开始, 0xFFDA) – 必要
* 编码图像数据 – 必要
* RSTn (重新启动次数, 0xFFDn) – 可选
* EOI (图片结尾, 0xFFD9) - 必要

图像数据需要以下内容：
* 颜色编码 - YCbCr
* 每像素位数 - 每个颜色分量8个（在过滤/二次采样之前）
* 二次抽样 - 422
* 基线顺序DCT (SOF0)
* 所有关键帧


## 设备描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x12 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x01 | DEVICE描述符
2 | bcdUSB | 2 | 0x0200 | 2.00 – USB规范的最新版本
4 | bDeviceClass | 1 | 0xEF | 杂项设备类别
5 | bDeviceSubClass | 1 | 0x02 | 通用类
6 | bDeviceProtocol | 1 | 0x01 | 接口关联描述符
7 | bMaxPacketSize0 | 1 | 0x40 | 控制端点数据包大小为64个字节
8 | idVendor | 2 | 0xXXXX | Vendor ID
10 | idProduct | 2 | 0xXXXX | Product ID
12 | bcdDevice | 2 | 0xXXXX | 设备发布代码
14 | iManufacturer | 1 | 0x01 | 包含Unicode中的字符串<您的名称>的字符串描述符的索引
15 | iProduct | 1 | 0x02 | 包含Unicode中的字符串<您的产品名称>的字符串描述符的索引
16 | iSerialNumber | 1 | 0x00 | 未使用
17 | bNumConfigurations | 1 | 0x01 | 一种配置

## 配置描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x09 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x02 | 配置描述符
2 | wTotalLength | 2 | 0x00C0 | 整个配置块的长度，包括此描述符，以字节为单位
4 | bNumInterfaces | 1 | 0x02 | 该设备有两个接口
5 | bConfigurationValue | 1 | 0x01 | 此配置的ID
6 | iConfiguration | 1 | 0x00 | 未使用
7 | bmAttributes | 1 | 0x80 | 总线供电的设备，没有远程唤醒功能
8 | bMaxPower | 1 | 0xFA | 500 mA最大功耗

## 标准视频接口集合IAD
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x08 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x0B | 接口关联描述符
2 | bFirstInterface | 1 | 0x00 | 与该功能关联的VideoControl接口的接口号
3 | bInterfaceCount | 1 | 0x02 | 与该功能关联的连续Video接口的数量
4 | bFunctionClass | 1 | 0x0E | CC_VIDEO
5 | bFunctionSubClass | 1 | 0x03 | SC_VIDEO_INTERFACE_COLLECTION
6 | bFunctionProtocol | 1 | 0x00 | 不曾用过。 必须设置为PC_PROTOCOL_UNDEFINED。
7 | iFunction | 1 | 0x02 | 包含Unicode中的字符串<您的产品名称>的字符串描述符的索引。必须匹配标准VC接口描述符中的iInterface字段。

## 标准VC接口描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x09 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x04 | 接口描述符类型
2 | bInterfaceNumber | 1 | 0x00 | 该接口的索引
3 | bAlternateSetting | 1 | 0x00 | 此设置的索引
4 | bNumEndpoints | 1 | 0x01 | 1个端点（中断端点）
5 | bInterfaceClass | 1 | 0x0E | CC_VIDEO
6 | bInterfaceSubClass | 1 | 0x01 | SC_VIDEOCONTROL
7 | bInterfaceProtocol | 1 | 0x01 | PC_PROTOCOL_15
8 | iInterface | 1 | 0x02 | 包含Unicode中的字符串<您的产品名称>的字符串描述符的索引。必须匹配标准视频接口集合IAD的iFunction字段。

## 特定于类的VC接口描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x0D | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubType | 1 | 0x01 | VC_HEADER subtype
3 | bcdUVC | 2 | 0x0150 | 修订此设备所基于的类规范。 对于此示例，设备符合视频类规范版本1.5。
5 | wTotalLength | 2 | 0x0042 | 类特定描述符的总大小
7 | dwClockFrequency | 4 | 0x005B8D80 | 不建议使用此字段。该设备将基于6MHz时钟提供时间戳和设备时钟参考。
11 | bInCollection | 1 | 0x01 | 流接口数量。
12 | baInterfaceNr(1) | 1 | 0x01 | 视频流接口1属于此视频控制接口。

## 输入端子描述符（相机）
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x11 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x02 | VC_INPUT_TERMINAL子类型
3 | bTerminalID | 1 | 0x01 | 该输入端子的ID
4 | wTerminalType | 2 | 0x0201 | ITT_CAMERA类型。 该端子是代表CMOS传感器的相机端子。
6 | bAssocTerminal | 1 | 0x00 | 不协商
7 | iTerminal | 1 | 0x00 | 未使用
8 | wObjectiveFocalLengthMin | 2 | 0x0000 | 不支持光学变焦
10 | wObjectiveFocalLengthMax | 2 | 0x0000 | 不支持光学变焦
12 | wOcularFocalLength | 2 | 0x0000 | 不支持光学变焦
14 | bControlSize | 1 | 0x02 | bmControls的大小为2个字节（此终端未实现任何控件）。
15 | bmControls | 2 | 0x0000 | 不支持任何控件。

## 输入端子描述符（复合）
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x08 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x02 | VC_INPUT_TERMINAL子类型
3 | bTerminalID | 1 | 0x02 | 该输入端子的ID
4 | wTerminalType | 2 | 0x0401 | COMPOSITE_CONNECTOR类型。 此端子是复合连接器。
6 | bAssocTerminal | 1 | 0x00 | 不协商
7 | iTerminal | 1 | 0x00 | 未使用

## 输出端子描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x09 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x03 | VC_OUTPUT_TERMINAL
3 | bTerminalID | 1 | 0x03 | 该终端的ID
4 | wTerminalType | 2 | 0x0101 | TT_STREAMING类型。 此终端是USB流终端。
6 | bAssocTerminal | 1 | 0x00 | 不协商
7 | bSourceID | 1 | 0x05 | 该单元的输入引脚连接到单元5的输出引脚。
8 | iTerminal | 1 | 0x00 | 未使用

## 选择器单元描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x08 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE描述符类型
2 | bDescriptorSubtype | 1 | 0x04 | VC_SELECTOR_UNIT描述符子类型
3 | bUnitID | 1 | 0x04 | 本机ID
4 | bNrInPins | 1 | 0x02 | 输入针数
5 | baSourceID(1) | 1 | 0x01 | 本机的输入1连接到单元ID 0x01 – CAMERA TERMINAL（CMOS传感器）。
6 | baSourceID(2) | 1 | 0x02 | 该单元的输入2连接到单元ID 0x02 –复合连接器。
7 | iSelector | 1 | 0x00 | 未使用

## 处理单元描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x0C | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x05 | VC_PROCESSING_UNIT
3 | bUnitID | 1 | 0x05 | 本机ID
4 | bSourceID | 1 | 0x04 | 该单元的该输入引脚连接到ID为0x04的单元的输出引脚。
5 | wMaxMultiplier | 2 | 0x0000 | 未使用
7 | bControlSize | 1 | 0x03 | bmControls字段的大小，以字节为单位。
8 | bmControls | 2 | 0x0001 | 支持亮度控制
10 | iProcessing | 1 | 0x00 | 未使用
11 | bmVideoStandards | 1 | 0x00 | 未使用

## 标准中断端点描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x07 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x05 | ENDPOINT描述符
2 | bEndpointAddress | 1 | 0x81 | IN端点1
3 | bmAttributes | 1 | 0x03 | 中断传输类型
4 | wMaxPacketSize | 2 | 0x40 | 64字节状态包
6 | bInterval | 1 | 0x20 | 至少每32毫秒轮询一次。

## 类特定的中断端点描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x05 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x05 | CS_ENDPOINT描述符
2 | bDescriptorSubType | 1 | 0x03 | EP_INTERRUPT
3 | wMaxTransferSize | 2 | 0x40 | 64字节状态包

## 标准VS接口描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x09 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x04 | 接口描述符类型
2 | bInterfaceNumber | 1 | 0x01 | 该接口的索引
3 | bAlternateSetting | 1 | 0x00 | 此备用设置的索引
4 | bNumEndpoints | 1 | 0x00 | 0个端点–不使用带宽
5 | bInterfaceClass | 1 | 0x0E | CC_VIDEO
6 | bInterfaceSubClass | 1 | 0x02 | SC_VIDEOSTREAMING
7 | bInterfaceProtocol | 1 | 0x00 | PC_PROTOCOL_15
8 | iInterface | 1 | 0x00 | 未使用

## 特定于类的VS Header描述符（输入）
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x0E | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x01 | VS_INPUT_HEADER.
3 | bNumFormats | 1 | 0x01 | 接下来是一种格式描述符。
4 | wTotalLength | 2 | 0x003F | 特定于类的VideoStreaming接口描述符的总大小
6 | bEndpointAddress | 1 | 0x82 | 用于视频数据的同步端点的地址
7 | bmInfo | 1 | 0x00 | 不支持动态格式更改
8 | bTerminalLink | 1 | 0x03 | 此VideoStreaming接口提供终端ID 3（输出终端）。
9 | bStillCaptureMethod | 1 | 0x01 | 设备支持静态图像捕获方法1。
10 | bTriggerSupport | 1 | 0x01 | 支持硬件触发器以进行静止图像捕获
11 | bTriggerUsage | 1 | 0x00 | 硬件触发器应启动静止图像捕获。
12 | bControlSize | 1 | 0x01 | bmaControls字段的大小
13 | bmaControls | 1 | 0x00 | 不支持特定于VideoStreaming的控件。

## 特定于类的VS格式描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x0B | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x06 | VS_FORMAT_MJPEG
3 | bFormatIndex | 1 | 0x01 | 第一个（也是唯一一个）格式描述符
4 | bNumFrameDescriptors | 1 | 0x01 | 随后是该格式的一帧描述符。
5 | bmFlags | 1 | 0x01 | 使用固定大小的样本。
6 | bDefaultFrameIndex | 1 | 0x01 | 默认帧索引为1。
7 | bAspectRatioX | 1 | 0x00 | 非隔行扫描流–不需要。
8 | bAspectRatioY | 1 | 0x00 | 非隔行扫描流–不需要。
9 | bmInterlaceFlags | 1 | 0x00 | 非交错流
10 | bCopyProtect | 1 | 0x00 | 对此视频流的复制没有任何限制。

## 特定于类的VS帧描述符
Offset | Field | Size | Value | Description
--- | --- | --- | --- | ---
0 | bLength | 1 | 0x26 | 此描述符的大小，以字节为单位。
1 | bDescriptorType | 1 | 0x24 | CS_INTERFACE
2 | bDescriptorSubtype | 1 | 0x07 | VS_FRAME_MJPEG
3 | bFrameIndex | 1 | 0x01 | 第一个（也是唯一一个）帧描述符
4 | bmCapabilities | 1 | 0x03 | 在此帧设置下支持使用拍摄方法1的静止图像。<br> D1: 固定帧率。
5 | wWidth | 2 | 0x00B0 | 画面宽度为176像素。
7 | wHeight | 2 | 0x0090 | 画面高度为144像素。
9 | dwMinBitRate | 4 | 0x000DEC00 | 最小比特率，单位为bit/s
13 | dwMaxBitRate | 4 | 0x000DEC00 | 最大比特率，单位为bit/s
17 | dwMaxVideoFrameBufSize | 4 | 0x00009480 | 最大视频或静止帧大小，以字节为单位。
21 | dwDefaultFrameInterval | 4 | 0x000A2C2A | 默认帧间隔为666666ns（15fps）。
25 | bFrameIntervalType | 1 | 0x00 | 连续帧间隔
26 | dwMinFrameInterval | 4 | 0x000A2C2A | 最小帧间隔为666666ns（15fps）
30 | dwMaxFrameInterval | 4 | 0x000A2C2A | 最大帧间隔为666666ns（15fps）
34 | dwFrameIntervalStep | 4 | 0x00000000 | 不支持帧间隔步长


