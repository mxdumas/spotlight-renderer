#include "UIRenderer.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "../Core/Config.h"
#include "../Scene/Spotlight.h"
#include "../Scene/CeilingLights.h"
#include "../Rendering/RenderPipeline.h"

UIRenderer::~UIRenderer() {
    Shutdown();
}

bool UIRenderer::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context) {
    if (m_initialized) {
        return true;
    }

    if (!device || !context) {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd)) {
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context)) {
        ImGui_ImplWin32_Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void UIRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    m_initialized = false;
}

void UIRenderer::BeginFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UIRenderer::RenderControls(UIContext& ctx) {
    if (!m_initialized) {
        return;
    }

    // Main Window
    ImGui::SetNextWindowPos(ImVec2(Config::UI::WINDOW_POS_X, Config::UI::WINDOW_POS_Y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(Config::UI::WINDOW_WIDTH, Config::UI::WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    ImGui::Begin("Spotlight Renderer Controls");

    // Performance Section
    ImGui::Text("Application Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Distance", ctx.camDistance, 0.1f, 1.0f, 200.0f);
        ImGui::SliderAngle("Pitch", ctx.camPitch, -89.0f, 89.0f);
        ImGui::SliderAngle("Yaw", ctx.camYaw, -180.0f, 180.0f);
        ImGui::DragFloat3("Target", &ctx.camTarget->x, 0.1f);
    }

    if (ImGui::CollapsingHeader("Spotlight Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Get mutable reference to spotlight data for ImGui controls
        SpotlightData& spotData = ctx.spotlight->GetGPUDataMutable();

        ImGui::Text("Environment");
        float ceilingInt = ctx.ceilingLights->GetIntensity();
        if (ImGui::SliderFloat("Ceiling Light Intensity", &ceilingInt, 1.0f, 100.0f)) {
            ctx.ceilingLights->SetIntensity(ceilingInt);
        }
        float ambFill = ctx.ceilingLights->GetAmbient();
        if (ImGui::SliderFloat("Ambient Fill", &ambFill, 0.0f, 100.0f)) {
            ctx.ceilingLights->SetAmbient(ambFill);
        }
        ImGui::SliderFloat("Room Specular", ctx.roomSpecular, 0.0f, 1.0f);
        ImGui::SliderFloat("Room Shininess", ctx.roomShininess, 1.0f, 128.0f);
        ImGui::Separator();

        ImGui::Text("Transform");
        ImGui::DragFloat3("Position", &spotData.posRange.x, 0.1f);
        ImGui::DragFloat3("Direction", &spotData.dirAngle.x, 0.01f);
        if (ImGui::Button("Reset to Fixture")) {
            ctx.spotlight->SetPosition(*ctx.fixturePos);
        }

        ImGui::Separator();
        ImGui::Text("Color & Intensity");
        ImGui::Checkbox("Use CMY Mixing", ctx.useCMY);
        if (*ctx.useCMY) {
            if (ImGui::ColorEdit3("CMY", &ctx.cmy->x)) {
                ctx.spotlight->SetColorFromCMY(ctx.cmy->x, ctx.cmy->y, ctx.cmy->z);
            }
        } else {
            ImGui::ColorEdit3("RGB Color", &spotData.colorInt.x);
        }
        ImGui::DragFloat("Intensity", &spotData.colorInt.w, 1.0f, 0.0f, 5000.0f);
        ImGui::DragFloat("Range", &spotData.posRange.w, 1.0f, 10.0f, 1000.0f);

        ImGui::Separator();
        ImGui::Text("Beam Shape");
        ImGui::SliderFloat("Beam Angle", &spotData.coneGobo.x, 0.0f, 1.0f);
        ImGui::SliderFloat("Field Angle", &spotData.coneGobo.y, 0.0f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Gobo Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        SpotlightData& spotData = ctx.spotlight->GetGPUDataMutable();
        ImGui::DragFloat("Rotation", &spotData.coneGobo.z, 0.01f);
        float shake = ctx.spotlight->GetGoboShake();
        if (ImGui::SliderFloat("Shake Amount", &shake, 0.0f, 1.0f)) {
            ctx.spotlight->SetGoboShake(shake);
        }
    }

    if (ImGui::CollapsingHeader("Volumetric Quality")) {
        VolumetricBuffer& volParams = ctx.pipeline->GetVolumetricParams();
        ImGui::DragFloat("Step Count", &volParams.params.x, 1.0f, Config::Volumetric::MIN_STEP_COUNT, Config::Volumetric::MAX_STEP_COUNT);
        ImGui::SliderFloat("Density", &volParams.params.y, 0.0f, 1.0f);
        ImGui::SliderFloat("Light Intensity Multiplier", &volParams.params.z, 0.0f, Config::Volumetric::DEFAULT_INTENSITY);
        ImGui::SliderFloat("Anisotropy (G)", &volParams.params.w, Config::Volumetric::MIN_ANISOTROPY, Config::Volumetric::MAX_ANISOTROPY);
    }

    if (ImGui::CollapsingHeader("Post Processing")) {
        bool fxaaEnabled = ctx.pipeline->IsFXAAEnabled();
        if (ImGui::Checkbox("Enable FXAA", &fxaaEnabled)) {
            ctx.pipeline->SetFXAAEnabled(fxaaEnabled);
        }
        bool blurEnabled = ctx.pipeline->IsVolumetricBlurEnabled();
        if (ImGui::Checkbox("Enable Volumetric Blur", &blurEnabled)) {
            ctx.pipeline->SetVolumetricBlurEnabled(blurEnabled);
        }
        int blurPasses = ctx.pipeline->GetBlurPasses();
        if (ImGui::SliderInt("Blur Passes", &blurPasses, Config::PostProcess::MIN_BLUR_PASSES, Config::PostProcess::MAX_BLUR_PASSES)) {
            ctx.pipeline->SetBlurPasses(blurPasses);
        }
    }

    ImGui::End();
}

void UIRenderer::EndFrame() {
    if (!m_initialized) {
        return;
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
