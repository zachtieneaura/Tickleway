// Tick Leeway - input buffer / coyote time (uso local y educativo)
// Objetivo: GD 2.2081 / Geode 5.10.1
// Si algun nombre de campo o firma no compila, revisa Geode/binding/*.hpp de tu SDK.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <imgui-cocos.hpp>
#include <algorithm>

using namespace geode::prelude;

// ---------- Estado compartido (solo hilo principal) ----------
namespace leeway {
    struct State {
        bool enabled = true;
        int  bufferTicks = 3;
        int  coyoteTicks = 0;

        // runtime
        bool bufferedPending = false;
        bool bufferedIsP1 = true;
        int  bufferedAge = 0;
        bool pendingRelease = false;  // release recibido mientras habia un click en buffer
        bool releaseNext = false;     // release a inyectar en el siguiente tick
        bool releaseNextIsP1 = true;
        int  ticksSinceGround = 999;
        int  lastTick = -1;
        int  lastUsedTicks = 0;       // para el HUD
        bool injecting = false;       // evita recursion al reenviar clicks
        bool usedThisAttempt = false; // para un futuro "modo seguro"
    };

    inline State& get() { static State s; return s; }

    inline void reset() {
        auto& s = get();
        s.bufferedPending = false; s.bufferedAge = 0;
        s.pendingRelease = false;  s.releaseNext = false;
        s.ticksSinceGround = 999;  s.lastTick = -1;
        s.lastUsedTicks = 0;       s.usedThisAttempt = false;
    }

    inline void syncSettings() {
        auto& s = get();
        s.enabled     = Mod::get()->getSettingValue<bool>("enabled");
        s.bufferTicks = static_cast<int>(Mod::get()->getSettingValue<int64_t>("buffer-ticks"));
        s.coyoteTicks = static_cast<int>(Mod::get()->getSettingValue<int64_t>("coyote-ticks"));
    }

    // Cubo/robot/arana necesitan suelo; nave, ufo, wave, swing no.
    inline bool needsGround(PlayerObject* p) {
        return !(p->m_isShip || p->m_isBird || p->m_isDart || p->m_isSwing);
    }

    // Contador de pasos de fisica. Verifica el nombre del campo en tus bindings.
    inline int currentTick(GJBaseGameLayer* gl) {
        return static_cast<int>(gl->m_gameState.m_currentProgress);
    }
}

// ---------- Logica de juego ----------
class $modify(LeewayGJBGL, GJBaseGameLayer) {

    void handleButton(bool down, int button, bool isPlayer1) {
        auto& s = leeway::get();

        if (!s.enabled || s.injecting || button != 1) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        // Release: si hay un click en buffer del mismo jugador, lo encolamos
        if (!down) {
            if (s.bufferedPending && s.bufferedIsP1 == isPlayer1) {
                s.pendingRelease = true;
                return;
            }
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        auto* p = isPlayer1 ? m_player1 : m_player2;
        if (!p || !leeway::needsGround(p) || p->m_isOnGround) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        // Coyote (experimental): acabamos de dejar el suelo
        if (s.coyoteTicks > 0 && s.ticksSinceGround <= s.coyoteTicks) {
            bool prev = p->m_isOnGround;
            p->m_isOnGround = true;
            s.injecting = true;
            GJBaseGameLayer::handleButton(true, button, isPlayer1);
            s.injecting = false;
            p->m_isOnGround = prev;
            s.lastUsedTicks = s.ticksSinceGround;
            s.usedThisAttempt = true;
            return;
        }

        // Buffer: guardamos el click en lugar de enviarlo ahora
        if (s.bufferTicks > 0) {
            s.bufferedPending = true;
            s.bufferedIsP1 = isPlayer1;
            s.bufferedAge = 0;
            s.pendingRelease = false;
            return;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt) {
        auto& s = leeway::get();

        if (s.enabled) {
            int tick = leeway::currentTick(this);
            int delta = (s.lastTick < 0) ? 0 : std::max(0, tick - s.lastTick);
            s.lastTick = tick;

            // Release diferido del click que se inyecto en el tick anterior
            if (s.releaseNext) {
                s.releaseNext = false;
                s.injecting = true;
                GJBaseGameLayer::handleButton(false, 1, s.releaseNextIsP1);
                s.injecting = false;
            }

            if (m_player1) {
                s.ticksSinceGround = m_player1->m_isOnGround ? 0 : s.ticksSinceGround + delta;
            }

            if (s.bufferedPending) {
                s.bufferedAge += delta;
                auto* p = s.bufferedIsP1 ? m_player1 : m_player2;

                if (p && (p->m_isOnGround || !leeway::needsGround(p))) {
                    s.lastUsedTicks = s.bufferedAge;
                    s.usedThisAttempt = true;
                    s.bufferedPending = false;

                    s.injecting = true;
                    GJBaseGameLayer::handleButton(true, 1, s.bufferedIsP1);
                    s.injecting = false;

                    if (s.pendingRelease) {
                        s.pendingRelease = false;
                        s.releaseNext = true;
                        s.releaseNextIsP1 = s.bufferedIsP1;
                    }
                } else if (s.bufferedAge > s.bufferTicks) {
                    s.bufferedPending = false; // expiro
                    s.pendingRelease = false;
                }
            }
        }

        GJBaseGameLayer::processCommands(dt);
    }
};

// ---------- HUD ----------
class $modify(LeewayPL, PlayLayer) {
    struct Fields {
        CCLabelBMFont* label = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto* l = CCLabelBMFont::create("", "bigFont.fnt");
        l->setScale(0.35f);
        l->setAnchorPoint({0.f, 1.f});
        auto win = CCDirector::get()->getWinSize();
        l->setPosition({8.f, win.height - 8.f});
        l->setOpacity(180);
        l->setID("leeway-hud"_spr);
        m_uiLayer->addChild(l);
        m_fields->label = l;

        leeway::reset();
        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& s = leeway::get();
        if (!m_fields->label) return;

        m_fields->label->setVisible(s.enabled);
        m_fields->label->setString(
            fmt::format("Buffer: {}/{} | Usado: {}t",
                        s.bufferedPending ? s.bufferedAge : 0,
                        s.bufferTicks,
                        s.lastUsedTicks).c_str());
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        leeway::reset();
    }
};

// ---------- Hotkey F6 ----------
class $modify(LeewayKB, CCKeyboardDispatcher) {
    // En 2.2081 la firma incluye `double timestamp`. Si no compila, quita ese parametro.
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat, double timestamp) {
        if (down && !repeat && key == KEY_F6) {
            ImGuiCocos::get().toggle();
            return true;
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat, timestamp);
    }
};

// ---------- Menu ImGui ----------
$on_mod(Loaded) {
    leeway::syncSettings();

    ImGuiCocos::get().setup([] {
        // estilo / fuentes opcionales
    }).draw([] {
        auto& s = leeway::get();
        ImGui::Begin("Tick Leeway");

        if (ImGui::Checkbox("Activado", &s.enabled)) {
            Mod::get()->setSettingValue<bool>("enabled", s.enabled);
        }
        if (ImGui::SliderInt("Ticks buffer", &s.bufferTicks, 0, 8)) {
            Mod::get()->setSettingValue<int64_t>("buffer-ticks", s.bufferTicks);
        }
        if (ImGui::SliderInt("Ticks coyote (exp.)", &s.coyoteTicks, 0, 8)) {
            Mod::get()->setSettingValue<int64_t>("coyote-ticks", s.coyoteTicks);
        }
        ImGui::Separator();
        ImGui::Text("Buffer activo: %s", s.bufferedPending ? "si" : "no");
        ImGui::Text("Ultimo uso: %d ticks", s.lastUsedTicks);
        ImGui::End();
    });
}
