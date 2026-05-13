#include "ScreenMessage.h"




/**
 * @brief 设置屏幕消息的显示文本
 * @param Message 要显示的字符串内容（FString类型）
 * @note 该函数属于UScreenMessage类的成员函数，用于更新UI文本控件的显示内容
 */
void UScreenMessage::SetMessageText(FString Message)
{
    // 将FString类型的字符串转换为Unreal UI系统推荐使用的FText类型
    // FText是本地化友好的文本类型，适合UI显示，而FString更适合逻辑处理
    FText MessageText = FText::FromString(Message);

    // 将转换后的FText文本设置到UI文本块控件（MessageTextBlock）上
    // MessageTextBlock是UI蓝图或代码中绑定的文本显示控件（如UMG的TextBlock）
    MessageTextBlock->SetText(MessageText);
}

// 实现变色
void UScreenMessage::SetMessageColor(FLinearColor Color)
{
    if (MessageTextBlock)
    {
        // FSlateColor 是 UI 专用的颜色结构，需要从 FLinearColor 转换
        MessageTextBlock->SetColorAndOpacity(FSlateColor(Color));
    }
}