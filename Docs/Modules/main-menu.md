# 主菜单模块开发者文档

更新日期：2026-06-07

## 职责

主菜单模块负责游戏入口、模式选择、本地多人入口、设置、地图选择和网络菜单入口。

主要目录：

```text
Source/BattleBlaster/Modes/MainMenu/
Source/BattleBlaster/Modes/MainMenu/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `AMainMenuGameMode` | 主菜单 GameMode，创建主菜单 UI |
| `UMainMenuWidget` | 主菜单按钮入口 |
| `UMutiPlayerMenuWidget` | 本地多人模式选择入口 |
| `USelectMapWidget` | 本地地图选择 |
| `UGameSettingsMenuWidget` | 设置菜单 |

## 当前设计要点

- 主菜单理论上是公共屏幕，不需要分屏。
- 本地多人只有进入选 Tank 菜单时才需要创建多个 LocalPlayer。
- 网络 Host / Join 前应确保本机只保留一个 LocalPlayer，否则容易出现 Server full 或 PIE 分屏混乱。

## 当前风险

- 主菜单、本地多人菜单、网络菜单之间的 LocalPlayer 管理必须保持清晰。
- 地图选择、本地模式选择、网络模式选择正在逐渐数据化，避免以后继续靠手填关卡名。
- 旧命名 `Muti` 已经存在于 UCLASS / 蓝图引用中，暂时不建议轻易改名。

## 推荐优化

建议把菜单跳转收口成一个轻量导航层：

```text
MenuNavigation
├── OpenLocalModeMenu
├── OpenLocalTankSelect
├── OpenLocalMapSelect
├── OpenNetworkModeSelect
├── OpenSettings
└── ReturnToMainMenu
```

如果暂时不加新类，也至少让所有 OpenLevel / CreateWidget 路径集中在少数函数里。

## Editor 注意

- 主菜单关卡的 GameMode 应使用主菜单专属 GameMode。
- 网络游戏按钮可以在 C++ 里提供 `OpenNetworkMenu()`，蓝图只绑定按钮和表现。
- 本地多人选 Tank 前要恢复多个 LocalPlayer；网络入口前要清理到一个 LocalPlayer。
