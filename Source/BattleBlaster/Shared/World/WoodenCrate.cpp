#include "Shared/World/WoodenCrate.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

void AWoodenCrate::HandleDestruction()
{
	// 1. 调用父类逻辑（关闭碰撞）
	Super::HandleDestruction();

	// 2. 隐藏原有的木箱模型
	PropMesh->SetVisibility(false);

	// 3. 播放破碎特效和音效
	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, GetActorLocation());
	}
	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}

	// 4. 等待几秒钟（让特效飞一会儿），然后彻底从内存中销毁这个Actor
	// 赋予这个木箱寿命，时间到了引擎会自动安全地把它 Destroy 掉
	SetLifeSpan(TimeToDisappear);
}