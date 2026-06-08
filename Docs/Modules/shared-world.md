# 地图交互物模块开发者文档

更新日期：2026-06-07

## 职责

地图交互物模块负责可破坏物、爆炸物、机关、传送门、滑轨、尖刺等场景 Actor。

主要目录：

```text
Source/BattleBlaster/Shared/World/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ADestructibleProp` | 可破坏物基类，生命值、血条、破坏表现、网络 transform 同步辅助 |
| `AExplosiveBarrel` | 爆炸油桶，可被推动、可造成范围伤害 |
| `AWoodenCrate` | 木箱，可破坏、可推动 |
| `ASpikeTrap` | 尖刺机关，服务器控制状态，客户端本地插值表现 |
| `ASlideTrack` | 滑道 / 加减速轨道 |
| `ARisingGate` | 升降门 |
| `ATeleportPortal` | 传送门 |

## 当前设计要点

- 可破坏物血条已经兼容本地分屏，不再只检查 Player 0。
- 油桶和木箱可以被推动，网络模式下需要同步位置。
- 尖刺不应该每帧由服务器同步位置；服务器同步状态和时间，客户端本地推演动画，这是更适合性能和观感的方案。

## 当前风险

- 物理推动物体在客户端可能出现低帧率感或卡顿，需要根据物体类型决定是 Replicate Movement、定时修正，还是客户端插值。
- 可破坏物、爆炸物、机关都可能对玩家造成伤害，需要统一接入 Combat Policy。
- Trigger Box 是否阻挡 Projectile 应由蓝图碰撞预设处理，不建议 Projectile 里写死特殊规则。

## 推荐优化

按交互物类型分别处理网络表现：

| 类型 | 推荐同步方式 |
| --- | --- |
| 静态可破坏物 | 同步生命值 / 破坏状态，位置通常不需要同步 |
| 可推动物体 | 服务器权威位置 + 客户端平滑插值 |
| 周期机关 | 同步状态、开始时间、持续时间，客户端本地动画 |
| 瞬时传送 | 服务器决定目标位置，客户端播放表现 |

可考虑新增一个轻量接口：

```text
INetworkedWorldActor
├── OnServerStateChanged
├── OnRep_WorldState
└── ApplyClientPresentation
```

## Editor 注意

- 可推动物体需要检查 StaticMesh 的物理和碰撞设置。
- Trigger Box 对 Projectile 应设置为 Overlap 或 Ignore。
- 尖刺、升降门、滑轨这类机关应优先暴露时间参数到蓝图，方便策划调手感。
