#include "Pch.h"
#include "Engine.h"
#include "Entityy.h"
#include "Core.h"

Engine::Engine()
{
    uint64_t base = MemoryManager.GetBaseAddress(ProcessName);
    if (base == 0) {
        printf("Engine ctor: Base address invalid\n");
        return;
    }

    GWorld = MemoryManager.Read<uint64_t>(base + GWorld);
    if (GWorld == 0) {
        printf("Engine ctor: Failed to read GWorld\n");
        return;
    }

    PersistentLevel = MemoryManager.Read<uint64_t>(GWorld + PersistentLevel);
    OwningGameInstance = MemoryManager.Read<uint64_t>(GWorld + OwningGameInstance);

    uint64_t localPlayersPtr = MemoryManager.Read<uint64_t>(OwningGameInstance + LocalPlayers);
    LocalPlayers = MemoryManager.Read<uint64_t>(localPlayersPtr);
    PlayerController = MemoryManager.Read<uint64_t>(LocalPlayers + PlayerController);
    CameraManager = MemoryManager.Read<uint64_t>(PlayerController + CameraManager);

    printf("Engine initialized - GWorld: 0x%llX\n", GWorld);
}

void Engine::Cache()
{
    if (PersistentLevel == 0) {
        printf("Cache skipped - PersistentLevel is 0\n");
        return;
    }

    struct TArray { uint64_t DataPointer; int Count; int Max; };
    TArray actorsArray = MemoryManager.Read<TArray>(PersistentLevel + 0xC0);

    if (actorsArray.DataPointer == 0 || actorsArray.Count <= 0 || actorsArray.Count > 100000) {
        printf("Invalid actor array (count: %d)\n", actorsArray.Count);
        return;
    }

    std::vector<uint64_t> entityList(actorsArray.Count);
    MemoryManager.Read(actorsArray.DataPointer, entityList.data(), actorsArray.Count * sizeof(uint64_t));

    std::list<std::shared_ptr<Entityy>> actors;
    auto handle = MemoryManager.CreateScatterHandle();

    for (uint64_t address : entityList)
    {
        if (address) actors.push_back(std::make_shared<Entityy>(address, handle));
    }
    MemoryManager.ExecuteReadScatter(handle);
    MemoryManager.CloseScatterHandle(handle);

    handle = MemoryManager.CreateScatterHandle();
    for (auto& entity : actors) entity->SetUp1(handle);
    MemoryManager.ExecuteReadScatter(handle);
    MemoryManager.CloseScatterHandle(handle);

    std::vector<std::shared_ptr<Entityy>> playerlist;
    for (auto& entity : actors)
    {
        entity->SetUp2();
        if (entity->GetName() == L"Survivor" || entity->GetName() == L"Killer")
        {
            playerlist.push_back(entity);
            printf("Found Actor: %ls at X:%.f Y:%.f\n", entity->GetName().c_str(), entity->GetPosition().x, entity->GetPosition().y);
        }
    }
    Actors = playerlist;
    printf("Total Players Cached: %zu\n", Actors.size());
}

void Engine::UpdatePlayers()
{
    auto handle = MemoryManager.CreateScatterHandle();
    for (auto& entity : Actors) entity->UpdatePosition(handle);
    MemoryManager.ExecuteReadScatter(handle);
    MemoryManager.CloseScatterHandle(handle);

    // Update local variables from the scatter results
    for (auto& entity : Actors)
    {
        UEVector pos = entity->GetUEPosition(); // Ensure you have a getter for this
        entity->SetPosition(Vector3((float)pos.X, (float)pos.Y, (float)pos.Z));
    }
}

void Engine::RefreshViewMatrix(VMMDLL_SCATTER_HANDLE handle)
{
    MemoryManager.AddScatterReadRequest(handle, CameraManager + CameraCachePrivateOffset, reinterpret_cast<void*>(&CameraEntry), sizeof(CameraCacheEntry));
}

CameraCacheEntry Engine::GetCameraCache() { return CameraEntry; }
std::vector<std::shared_ptr<Entityy>> Engine::GetActors() { return Actors; }
uint32_t Engine::GetActorSize() { return (uint32_t)Actors.size(); }