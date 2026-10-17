#include "game/Pickup.h"

#include "gfx/Assets.h"

namespace ls {

const wchar_t* pickupLabel(PickupKind kind)
{
    switch (kind) {
    case PickupKind::Speed: return L"移动速度 +";
    case PickupKind::Damage: return L"子弹伤害 +";
    case PickupKind::FireRate: return L"射速 +";
    case PickupKind::Health: return L"生命 +";
    case PickupKind::Grenade: return L"手雷 +";
    default: return L"";
    }
}

const Sprite& pickupSprite(PickupKind kind)
{
    const Assets& a = assets();
    switch (kind) {
    case PickupKind::Speed: return a.itemSpeed;
    case PickupKind::Damage: return a.itemAttack;
    case PickupKind::FireRate: return a.itemRate;
    case PickupKind::Health: return a.itemHeart;
    case PickupKind::Grenade: return a.itemGrenade;
    default: return a.itemSpeed;
    }
}

Rgba pickupColor(PickupKind kind)
{
    switch (kind) {
    case PickupKind::Speed: return palette::kAccent;
    case PickupKind::Damage: return palette::kGold;
    case PickupKind::FireRate: return rgba(140, 200, 255);
    case PickupKind::Health: return palette::kHeal;
    case PickupKind::Grenade: return rgba(255, 176, 96);
    default: return palette::kWhite;
    }
}

} // namespace ls
