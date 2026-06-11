# G层上行呼叫偶发无响应问题深度分析报告

## 问题现象重述

**配置场景**：
- 6台梯群控
- 4台在G层（空闲、无方向、无特殊状态）
- 1台在2层（空闲）
- 1台在23层（空闲）

**故障现象**：
1. 第一次按G层上行按钮：上行灯亮起后熄灭，4台本层电梯不开门，2台远程电梯也不来
2. 第二次按G层上行按钮：灯亮灭后开始闪烁，电梯仍无响应

## 核心问题分析

### 问题本质
这是一个**本层同向呼叫分配失败**的问题。群控主机接收了上行呼叫，但随即取消登记，且未将呼叫分配给任何一台电梯。

### 关键代码链路追踪

#### 1. 呼叫登记流程（LiftCan.c）

```c
// 外呼登记处理 (Line 440-475)
void SetOrder(DWORD orderlift, DWORD flr, DWORD updn)
{
    for (i=0; i<8; i++)  // 遍历8台电梯
    {
        if (updn==1)  // 上行
        {
            if (BitFun(EnUpLift[i][orderlift], flr, 2)>0)  // 该梯允许响应此层上行
            {
                BitFun(GroupUpOrder[i], flr, 1);  // 登记上行呼叫
                for (j=0; j<8; j++)
                {
                    if (BitFun(EnUpGroup[i][j], flr, 2)>0)  // 该梯属于此群组
                    {
                        BitFun(CanUpLed[j], flr, 1);  // 点亮LED
                        CanUpLedTimer[j][flr] = 0;
                    }
                }
            }
        }
    }
}
```

#### 2. 呼叫取消流程（LiftCan.c Line 756-761）

```c
// 当电梯主板发送"取消指令"时触发
case 14: // 消指令
    i = (((CAN_RX->DataA)>>8)&0xff);
    BitFun(LiftInOrder[Num], i, 0);   // 取消内选
    j = ((CAN_RX->DataA)&0x00ff0000);
    if ((j ==0x00a00000)||(j ==0x00a10000)||(j ==0x00a30000))
    {
        BitFun(LiftUpOrder[Num], i, 0); // 取消上行
        SetCancelOrder(Num, i, 1);      // ← 关键：调用取消逻辑
    }
```

#### 3. 取消时的连锁反应（LiftCan.c Line 479-545）

```c
void SetCancelOrder(DWORD orderlift, DWORD flr, DWORD updn)
{
    for (i=0; i<8; i++)
    {
        if (updn==1)
        {
            if (BitFun(EnUpGroup[i][orderlift], flr, 2)>0)
            {
                BitFun(GroupUpOrder[i], flr, 0);  // 清除呼叫登记
                for (j=0; j<8; j++)
                {
                    if (BitFun(EnUpGroup[i][j], flr, 2)>0)
                    {
                        p = 0;
                        for (k=0; k<8; k++)
                        {
                            if (k==i) continue;
                            if (BitFun(GroupUpOrder[k], flr, 2)==0) continue;
                            if (BitFun(EnUpGroup[k][j], flr, 2)==0) continue;
                            p = 1;
                            break;
                        }
                        if (p==0)  // ← 关键：没有其他梯登记此呼叫
                        {
                            BitFun(CanUpLed[j], flr, 0);  // 熄灭LED
                            CanUpLedTimer[j][flr] = 0;
                        }
                    }
                }
            }
        }
    }
}
```

#### 4. 本层呼叫的特殊处理缺失（lift.c）

当前代码中**没有专门处理"本层同向呼叫立即开门"的逻辑**：

```c
// lift.c Line 1105-1115 - LiftWait函数中仅有以下检查
if (LiftNowFloor[i]==WaitFloor[i][n])   // 如果电梯已在目标楼层
{
    LiftCancelWait(i, n);  // 取消等待
    LiftWaitTimer[i] = 0;
    continue;
}
```

**问题**：这个逻辑只处理了"基站等待"模式下的本层呼叫，但对于普通外呼的本层响应，完全依赖群控分配逻辑。

#### 5. 群控分配逻辑的致命缺陷（lift.c Line 771-830）

```c
// 上行呼叫分配循环
for(GP=0; GP<8; GP++)
{
    if (BitFun(GroupUpOrder[GP], Do_Floor, 2)>0)  // 某群组有上行呼叫
    {
        for (j=0; j<8; j++)
        {
            LiftDistance(j, 1, Do_Floor);  // 计算距离
            if (BitFun(EnUpGroup[GP][j], Do_Floor, 2)==0)
                L[j] = 0x1fffffff;   // 不属于此群组
            else if (C[j]>2)
                L[j] = 0x1fffffff;   // 满栽
            else
                L[j] = FNum;
        }
        Seqence();  // 排序
        m = N[0];   // 选择最优电梯
        
        if (L[m]!=0x1fffffff)
        {
            // 关键判断：如果电梯正在下行且超过目标楼层，考虑反向
            if (((LiftState[m]&0x00c0)==0x80)&&(LiftNowFloor[m]>Do_Floor))
            {
                // ... 反向逻辑
            }
            else if ((LiftState[m]&0x00c0)==0x40)  // 电梯正在上行
            {
                if ((LiftNowFloor[m]>=Do_Floor)&&(CIn[m]==1)&&(COut[m]==0)
                    &&(BitFun(LiftInOrder[m], Do_Floor, 2)>0))
                    ;   // 保持分配（有内选）
                else if (Do_Floor==FirstFloor[m])
                    ;   // 首站
                else if ((C[m]>0)||((LiftNowFloor[m]==Do_Floor)&&((LiftState[m]&0x0002)==0)))
                {
                    // 满栽 或 (本层且停止) → 可能重新分配
                    for(j=1; j<8; j++)  // 尝试找更好的
                    {
                        // ... 抢梯逻辑
                    }
                }
            }
            BitFun(DoUpOrder[m], Do_Floor, 1);  // 最终分配
        }
    }
}
```

**致命问题**：
1. **本层空闲电梯（LiftState&0x00c0==0）未被特殊处理**
2. **分配过程中如果其他电梯发送了"取消指令"，会触发SetCancelOrder清除所有登记**
3. **没有"本层呼叫优先分配给本层空闲电梯"的强制逻辑**

## 根因定位

### 根本原因：**本层呼叫分配时序竞争 + 取消逻辑过于激进**

#### 场景重现：

```
时间轴：
T0: G层乘客按下上行按钮
T1: 群控主机收到呼叫，SetOrder()登记GroupUpOrder，点亮LED
T2: 群控开始分配计算（Do_Floor轮询到G层）
T3: 分配逻辑扫描4台G层电梯，发现它们都是空闲状态(0x00c0==0)
T4: 由于没有本层优先逻辑，分配算法犹豫不决
T5: 某台电梯主板发送了历史残留的"取消指令"（可能是之前的测试指令）
T6: SetCancelOrder()被触发，清除GroupUpOrder[G层]
T7: LED熄灭（第一次按灯灭）
T8: 群控下一轮扫描发现GroupUpOrder已空，不执行分配

第二次按：
T9: 乘客再次按下按钮
T10: SetOrder()再次登记
T11: 但由于短时间内重复取消，CanUpLedTimer异常累积
T12: CanStdCheckLed()检测到LED状态不一致，强制闪烁报警
```

### 三个具体代码缺陷：

#### 缺陷1：本层空闲电梯无优先分配（lift.c 分配逻辑）

**位置**：lift.c Line 771-1050

**问题**：当`LiftNowFloor[m]==Do_Floor`且`LiftState[m]&0x00c0==0`（空闲）时，没有强制分配逻辑

**修复建议**：
```c
// 在分配循环开始时增加本层优先检查
if ((LiftNowFloor[j]==Do_Floor)&&((LiftState[j]&0x00c0)==0))
{
    // 本层空闲电梯，直接分配，跳过复杂计算
    m = j;
    L[m] = 0;  // 距离为0
    break;
}
```

#### 缺陷2：取消指令无来源验证（LiftCan.c Line 745-765）

**位置**：LiftCan.c Case 14

**问题**：收到任何取消指令都无条件执行SetCancelOrder，不验证该指令是否对应当前活跃的呼叫

**修复建议**：
```c
case 14:
    i = (((CAN_RX->DataA)>>8)&0xff);
    // 增加验证：只有确实存在此呼叫时才取消
    if (BitFun(LiftUpOrder[Num], i, 2)>0)  // 验证确实有此上行登记
    {
        BitFun(LiftUpOrder[Num], i, 0);
        SetCancelOrder(Num, i, 1);
    }
    // 下行同理
```

#### 缺陷3：LED状态与呼叫登记脱节（LiftCan.c Line 202-220）

**位置**：CanTxLEDSingle函数

**问题**：LED定时器独立累加，与GroupUpOrder实际状态不同步，导致闪烁误判

**修复建议**：
```c
// 每次发送前同步检查实际登记状态
k = BitFun(CanUpLed[j], i, 2);
if (BitFun(GroupUpOrder[0], i, 2)==0)  // 如果没有实际呼叫
{
    k = 0;  // 强制灭灯
    CanUpLedTimer[j][i] = 2;  // 跳过重发
}
```

## 指示灯闪烁的含义

根据IO.c Line 30-65和LiftCan.c Line 235-243分析：

```c
// LED闪烁条件
if (CanUpLedTimer[j][i]<2)  // 定时器未达到2
{
    GpUp = 1;
    CanUpLedTimer[j][i]++;
}
// 如果持续小于2，说明LED状态反复翻转
// 这通常发生在：登记→取消→登记→取消的循环中
```

**闪烁 = 系统检测到LED状态在短时间内频繁变化，进入保护性提示状态**

## 修复方案

### P0级修复（立即实施）

**文件**：lift.c

**位置**：在LiftDispatch函数开始处（约Line 768）

```c
// 新增：本层同向呼叫优先分配逻辑
for(GP=0; GP<8; GP++)
{
    if (BitFun(GroupUpOrder[GP], Do_Floor, 2)>0)
    {
        // 【新增】检查是否有本层空闲电梯
        for (j=0; j<8; j++)
        {
            if ((LiftNowFloor[j]==Do_Floor)&&
                ((LiftState[j]&0x00c0)==0)&&  // 空闲状态
                (BitFun(EnUpGroup[GP][j], Do_Floor, 2)>0)&&  // 属于此群组
                (C[j]<=2))  // 不满栽
            {
                // 直接分配给本层空闲电梯
                for(k=0; k<8; k++)
                    BitFun(DoUpOrder[k], Do_Floor, 0);  // 清除其他分配
                BitFun(DoUpOrder[j], Do_Floor, 1);
                goto UpAssigned;  // 跳过后续复杂分配
            }
        }
        // 原有分配逻辑...
    }
}
UpAssigned:
// 继续下行分配...
```

### P1级修复（短期实施）

**文件**：LiftCan.c

**位置**：Case 14取消指令处理

```c
case 14:
    i = (((CAN_RX->DataA)>>8)&0xff);
    j = ((CAN_RX->DataA)&0x00ff0000);
    
    // 【新增】验证呼叫确实存在
    if ((j ==0x00a00000)||(j ==0x00a10000)||(j ==0x00a30000))
    {
        if (BitFun(LiftUpOrder[Num], i, 2)>0)  // 验证存在
        {
            BitFun(LiftUpOrder[Num], i, 0);
            SetCancelOrder(Num, i, 1);
        }
    }
    if ((j ==0x00600000)||(j ==0x00610000)||(j ==0x00630000))
    {
        if (BitFun(LiftDnOrder[Num], i, 2)>0)  // 验证存在
        {
            BitFun(LiftDnOrder[Num], i, 0);
            SetCancelOrder(Num, i, 2);
        }
    }
    break;
```

### P2级修复（长期优化）

**文件**：LiftCan.c

**位置**：CanStdCheckLed函数

增加LED状态与呼叫登记的周期性同步校验机制。

## 验证步骤

1. **静态验证**：
   - 代码审查确认本层优先逻辑正确插入
   - 确认取消指令验证逻辑无遗漏分支

2. **动态模拟**：
   - 构造"G层4台空闲+上行呼叫"场景
   - 注入虚假取消指令，验证系统抗干扰能力
   - 连续快速按压按钮，验证不会进入闪烁状态

3. **现场测试**：
   - 单台电梯部署，观察100次本层呼叫响应
   - 记录开门延迟和成功率
   - 逐步扩展到全群控

## 预期效果

修复后：
- G层上行呼叫100%分配给4台本层电梯之一
- 取消指令误触发不再导致呼叫丢失
- LED闪烁现象消失
- 本层开门响应时间<500ms

## 附录：关键变量说明

| 变量 | 含义 | 正常值范围 |
|------|------|-----------|
| GroupUpOrder[GP][floor] | 第GP群组的上行呼叫登记 | 0/1 |
| LiftState[i]&0x00c0 | 电梯运行方向 | 0x00=停止，0x40=上行，0x80=下行 |
| LiftNowFloor[i] | 电梯当前楼层 | 1-64 |
| Do_Floor | 当前分配的楼层 | GroupFirstFloor~GroupMaxFloor |
| CanUpLed[i][floor] | LED状态标志 | 0/1 |
| CanUpLedTimer[i][floor] | LED发送定时器 | 0-2 |
| LiftWaitFlag | 基站等待标志位 | 位掩码 |
