#include "../../../includes.h"

void Hooks::SceneEnd()
{
    g_hooks.m_render_view.GetOldMethod< Hooks::SceneEnd_t >(IVRenderView::SCENEEND)(this);

    bool once = false;
    if (!once)
    {
        std::ofstream("csgo\materials\regular_glow.vmt") << R"#("VertexLitGeneric"{
            "$additive" "1"
            "$envmap" "models/effects/cube_white"
            "$envmaptint" "[1 1 1]"
            "$envmapfresnel" "1"
            "$envmapfresnelminmaxexp" "[0 1 2]"
            "$alpha" "1"
        })#";
        once = true;
    }
    g_chams.esoglow = g_csgo.m_material_system->FindMaterial("regular_glow", nullptr);
    g_chams.esoglow->IncrementReferenceCount();

    g_chams.glowarmsrace = g_csgo.m_material_system->FindMaterial("dev/glow_armsrace", nullptr);
    g_chams.glowarmsrace->IncrementReferenceCount();

    g_chams.SceneEnd();
}