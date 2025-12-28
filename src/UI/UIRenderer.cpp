#include "UIRenderer.h"
#include "../Core/Config.h"
#include "../Rendering/RenderPipeline.h"
#include "../Scene/Scene.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

UIRenderer::~UIRenderer()
{
    Shutdown();
}

bool UIRenderer::Initialize(HWND hwnd, ID3D11Device *device, ID3D11DeviceContext *context)
{
    if (m_initialized)
    {
        return true;
    }

    if (!device || !context)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd))
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context))
    {
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void UIRenderer::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (ImGui::GetCurrentContext())
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    m_initialized = false;
}

void UIRenderer::BeginFrame()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UIRenderer::RenderControls(UIContext &ctx)
{
    if (!m_initialized || !ctx.scene)
    {
        return;
    }

    Scene &scene = *ctx.scene;

    // Main Window
    ImGui::SetNextWindowPos(ImVec2(Config::UI::WINDOW_POS_X, Config::UI::WINDOW_POS_Y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(Config::UI::WINDOW_WIDTH, Config::UI::WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    ImGui::Begin("Spotlight Renderer Controls");

    // Performance Section
    ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragFloat("Distance", &scene.CamDistance(), 0.1f, 1.0f, 200.0f);
        ImGui::SliderAngle("Pitch", &scene.CamPitch(), -89.0f, 89.0f);
        ImGui::SliderAngle("Yaw", &scene.CamYaw(), -180.0f, 180.0f);
        ImGui::DragFloat3("Target", &scene.CamTarget().x, 0.1f);
    }

    if (ImGui::CollapsingHeader("Demo Effects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        EffectsEngine &effects = scene.GetEffectsEngine();

        ImGui::Checkbox("Enable Demo", &effects.Enabled());

        if (effects.IsEnabled())
        {
            ImGui::SliderFloat("Speed", &effects.Speed(), 0.1f, 3.0f, "%.1fx");
            ImGui::Checkbox("Pan", &effects.PanEnabled());
            ImGui::SameLine();
            ImGui::Checkbox("Tilt", &effects.TiltEnabled());
            ImGui::SameLine();
            ImGui::Checkbox("Rainbow", &effects.RainbowEnabled());
            ImGui::Checkbox("Gobo Rotation", &effects.GoboRotationEnabled());
        }
    }

    if (ImGui::CollapsingHeader("Global Scene Parameters", ImGuiTreeNodeFlags_DefaultOpen))
    {
        CeilingLights &ceiling_lights = scene.GetCeilingLights();

        ImGui::Text("Environment");
        float ceiling_int = ceiling_lights.GetIntensity();
        if (ImGui::SliderFloat("Ceiling Light Intensity", &ceiling_int, 1.0f, 100.0f))
        {
            ceiling_lights.SetIntensity(ceiling_int);
        }
        float amb_fill = ceiling_lights.GetAmbient();
        if (ImGui::SliderFloat("Ambient Fill", &amb_fill, 0.0f, 100.0f))
        {
            ceiling_lights.SetAmbient(amb_fill);
        }
        ImGui::SliderFloat("Room Specular", &scene.RoomSpecular(), 0.0f, 1.0f);
        ImGui::SliderFloat("Room Shininess", &scene.RoomShininess(), 1.0f, 128.0f);
    }

    auto &spotlights = scene.GetSpotlights();
    for (size_t i = 0; i < spotlights.size(); ++i)
    {
        char label[32];
        sprintf_s(label, "Spotlight %zu", i + 1);
        if (ImGui::CollapsingHeader(label))
        {
            Spotlight &spotlight = spotlights[i];
            SpotlightData &spot_data = spotlight.GetGPUDataMutable();

            ImGui::PushID(static_cast<int>(i));

            ImGui::Text("GDTF Orientation");
            float pan = spotlight.GetPan();
            if (ImGui::SliderFloat("Pan", &pan, -180.0f, 180.0f))
            {
                spotlight.SetPan(pan);
            }
            float tilt = spotlight.GetTilt();
            if (ImGui::SliderFloat("Tilt", &tilt, -90.0f, 90.0f))
            {
                spotlight.SetTilt(tilt);
            }

            ImGui::Separator();
            ImGui::Text("Color & Intensity");
            if (ImGui::ColorEdit3("Color", &spot_data.colorInt.x))
            {
                // Sync CMY if needed or just use RGB
            }
            ImGui::DragFloat("Intensity", &spot_data.colorInt.w, 1.0f, 0.0f, 5000.0f);
            ImGui::DragFloat("Range", &spot_data.posRange.w, 1.0f, 10.0f, 1000.0f);

            ImGui::Separator();
            ImGui::Text("Beam Shape");
            ImGui::SliderFloat("Beam Angle", &spot_data.coneGobo.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Field Angle", &spot_data.coneGobo.y, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Text("Gobo Settings");

            const auto &gobo_names = scene.GetGoboSlotNames();
            if (!gobo_names.empty())
            {
                int current_gobo = spotlight.GetGoboIndex();
                std::string current_gobo_name =
                    (current_gobo >= 0 && static_cast<size_t>(current_gobo) < gobo_names.size())
                        ? gobo_names[static_cast<size_t>(current_gobo)]
                        : "Unknown";

                if (ImGui::BeginCombo("Gobo", current_gobo_name.c_str()))
                {
                    for (size_t n = 0; n < gobo_names.size(); n++)
                    {
                        const bool is_selected = (static_cast<size_t>(current_gobo) == n);
                        if (ImGui::Selectable(gobo_names[n].c_str(), is_selected))
                        {
                            spotlight.SetGoboIndex(static_cast<int>(n));
                        }
                        if (is_selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::DragFloat("Gobo Rotation", &spot_data.coneGobo.z, 0.01f);
            float shake = spotlight.GetGoboShake();
            if (ImGui::SliderFloat("Shake Amount", &shake, 0.0f, 1.0f))
            {
                spotlight.SetGoboShake(shake);
            }

            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Volumetric Quality"))
    {
        VolumetricBuffer &vol_params = ctx.pipeline->GetVolumetricParams();
        ImGui::DragFloat("Step Count", &vol_params.params.x, 1.0f, Config::Volumetric::MIN_STEP_COUNT,
                         Config::Volumetric::MAX_STEP_COUNT);
        ImGui::SliderFloat("Density", &vol_params.params.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Intensity Multiplier", &vol_params.params.z, 0.0f,
                           Config::Volumetric::DEFAULT_INTENSITY);
        ImGui::SliderFloat("Anisotropy (G)", &vol_params.params.w, Config::Volumetric::MIN_ANISOTROPY,
                           Config::Volumetric::MAX_ANISOTROPY);
    }

    if (ImGui::CollapsingHeader("Post Processing"))
    {
        bool fxaa_enabled = ctx.pipeline->IsFXAAEnabled();
        if (ImGui::Checkbox("Enable FXAA", &fxaa_enabled))
        {
            ctx.pipeline->SetFXAAEnabled(fxaa_enabled);
        }
        bool blur_enabled = ctx.pipeline->IsVolumetricBlurEnabled();
        if (ImGui::Checkbox("Enable Volumetric Blur", &blur_enabled))
        {
            ctx.pipeline->SetVolumetricBlurEnabled(blur_enabled);
        }
        int blur_passes = ctx.pipeline->GetBlurPasses();
        if (ImGui::SliderInt("Blur Passes", &blur_passes, Config::PostProcess::MIN_BLUR_PASSES,
                             Config::PostProcess::MAX_BLUR_PASSES))
        {
            ctx.pipeline->SetBlurPasses(blur_passes);
        }
    }

    ImGui::End();
}

void UIRenderer::EndFrame()
{
    if (!m_initialized)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
