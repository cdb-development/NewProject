#include "Pch.h"
#include "Entityy.h"
#include "Camera.h"
#include "Core.h"
#include "Render.h"  // For Render lines

Entityy::Entityy(uint64_t address, VMMDLL_SCATTER_HANDLE handle)
{
    Class = address;
    if (!address) return;

    MemoryManager.AddScatterReadRequest(handle, Class + PlayerState, reinterpret_cast<void*>(&PlayerState), sizeof(uint64_t));
    MemoryManager.AddScatterReadRequest(handle, Class + RootComponent, reinterpret_cast<void*>(&RootComponent), sizeof(uint64_t));

    // Mesh component for skeleton (common offset ~0x410, verify with dumper)
    MemoryManager.AddScatterReadRequest(handle, Class + 0x410, reinterpret_cast<void*>(&MeshComponent), sizeof(uint64_t));
}

void Entityy::SetUp1(VMMDLL_SCATTER_HANDLE handle)
{
    if (!Class || !RootComponent) return;

    if (PlayerState)
    {
        MemoryManager.AddScatterReadRequest(handle, PlayerState + GameRole, reinterpret_cast<void*>(&PlayerRole), sizeof(EPlayerRole));
    }
}

void Entityy::SetUp2()
{
    if (!Class || !RootComponent) return;

    if (PlayerState)
    {
        if (PlayerRole != EPlayerRole::EPlayerRole__VE_Camper && PlayerRole != EPlayerRole::EPlayerRole__VE_Slasher)
            return;

        Name = (PlayerRole == EPlayerRole::EPlayerRole__VE_Camper) ? LIT(L"Survivor") : LIT(L"Killer");

        UEPosition = MemoryManager.Read<UEVector>(RootComponent + RelativeLocation);
        Position = Vector3((float)UEPosition.X, (float)UEPosition.Y, (float)UEPosition.Z);
    }
}

void Entityy::UpdatePosition(VMMDLL_SCATTER_HANDLE handle)
{
    if (!Class || !RootComponent || !PlayerState) return;

    if (PlayerRole == EPlayerRole::EPlayerRole__VE_Camper || PlayerRole == EPlayerRole::EPlayerRole__VE_Slasher)
    {
        MemoryManager.AddScatterReadRequest(handle, RootComponent + RelativeLocation, reinterpret_cast<void*>(&UEPosition), sizeof(UEVector));
    }
}

// New: Update Bones
void Entityy::UpdateBones(VMMDLL_SCATTER_HANDLE handle)
{
    if (!MeshComponent) return;

    uint64_t TransformsArray = MemoryManager.Read<uint64_t>(MeshComponent + 0x0AC8);

    if (!TransformsArray) return;

    if ((Name == L"Survivor" || Name == L"Killer") && (rand() % 20 == 0)) {  // reduce spam
        printf("CachedComponentSpaceTransforms found at 0x%llX for %ls\n", TransformsArray, Name.c_str());
    }

    std::vector<int> importantBones = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 20 };

    BonePositions.clear();
    BonePositions.reserve(importantBones.size());

    for (int idx : importantBones)
    {
        uint64_t boneAddr = TransformsArray + (uint64_t)idx * sizeof(FTransform);  // 0x30 or 0x40 usually
        FTransform transform = MemoryManager.Read<FTransform>(boneAddr);

        Vector3 pos(transform.Translation.X, transform.Translation.Y, transform.Translation.Z);

        // Print only non-zero bones
        if (fabs(pos.x) > 10.0f || fabs(pos.y) > 10.0f || fabs(pos.z) > 10.0f) {
            printf("Bone %d: %.1f, %.1f, %.1f\n", idx, pos.x, pos.y, pos.z);
        }

        BonePositions.push_back(pos);
    }
}

// New: Draw Skeleton
void Entityy::DrawSkeleton()
{
    if (BonePositions.size() < 5) return;

    MyColour color = (PlayerRole == EPlayerRole::EPlayerRole__VE_Slasher) ? MyColour(255, 0, 0, 255) : MyColour(0, 255, 0, 255);

    // Basic connections (tune indices based on actual bones)
    auto drawBoneLine = [&](int a, int b) {
        if (a >= BonePositions.size() || b >= BonePositions.size()) return;
        Vector2 s1 = Camera::WorldToScreen(EngineInstance->GetCameraCache().POV, BonePositions[a]);
        Vector2 s2 = Camera::WorldToScreen(EngineInstance->GetCameraCache().POV, BonePositions[b]);
        if (s1 != Vector2::Zero() && s2 != Vector2::Zero()) {
            FilledLine(s1.x, s1.y, s2.x, s2.y, 2, color);
        }
        };

    // Example skeleton lines (very basic - improve this)
    drawBoneLine(0, 1);  // Head -> Neck
    drawBoneLine(1, 2);  // Neck -> Spine
    drawBoneLine(2, 3);  // Spine -> Pelvis
    // Add arms, legs, etc.
}

Vector3 Entityy::GetBoneWorldPosition(int boneIndex)
{
    if (boneIndex < 0 || boneIndex >= BonePositions.size()) return Vector3(0, 0, 0);
    return BonePositions[boneIndex];
}

// Getters (unchanged)
EPlayerRole Entityy::GetPlayerRole() { return PlayerRole; }
uint64_t Entityy::GetClass() { return Class; }
std::wstring Entityy::GetName() { return Name; }
Vector3 Entityy::GetPosition() { return Position; }
UEVector Entityy::GetUEPosition() { return UEPosition; }
void Entityy::SetPosition(Vector3 pos) { Position = pos; }