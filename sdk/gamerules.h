#pragma once

class C_CSGameRules
{
public:
    __forceinline C_CSGameRules* get_gamerules()
    {
        if (!g_csgo.m_engine->IsInGame())
            return nullptr;

        return Address(this).to<C_CSGameRules*>();
    }

private:
    template<typename t>
    __forceinline t get(Address offset)
    {
        C_CSGameRules* gr{get_gamerules()};

        if (!gr)
            return t{};

        return *(t*)((uintptr_t)gr + offset);
    }
public:
    __forceinline bool m_bFreezePeriod()
    {
        return get<bool>(0x20);
    }
};