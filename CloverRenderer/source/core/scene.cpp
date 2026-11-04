#include "core/scene.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <core/transform.hpp>
#include <rendering/renderer.hpp>
#include "entt/meta/meta.hpp"
#include "entt/core/hashed_string.hpp"
#include <fstream>
#include <core/selection_gizmo.hpp>
#include <core/translate_gizmo.hpp>
#include <unordered_map>

using namespace entt::literals;
using namespace clvr;

SceneManager::SceneManager() : EditorWindow("Outliner")
{
    // Create the selection gizmo
    m_selectionGizmo = std::make_unique<SelectionGizmo>();
    // Create the translate gizmo
    //m_translateGizmo = std::make_unique<TranslateGizmo>();

    Open();
}

void SceneManager::Update(float dt)
{
    // Create a default sprite layer for the gizmos
    auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
    auto gizmoLayer = renderer.FindOrCreateSpriteLayer(-1);
    gizmoLayer->hiddenInInspector = true;
    gizmoLayer->generated = true;

    if (m_selectionGizmo)
		m_selectionGizmo->Update(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
    return; // translate gizmos disabled for now 
	if (m_translateGizmo)
		m_translateGizmo->Update(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Render()
{
    return; // gizmos disabled for now
	if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Draw()
{
    if (m_selectionGizmo)
		m_selectionGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
    return; // gizmos disabled for now
	if (m_translateGizmo)
		m_translateGizmo->Draw(Engine.GetECS()->GetSystem<Renderer>().GetActiveCamera());
}

void SceneManager::Inspect(float dt)
{
    auto* ecs = Engine.GetECS();
	auto& registry = ecs->GetRegistry();
	auto& renderer = ecs->GetSystem<Renderer>();

    EditorWindow::DrawWindow();

    ImGui::Begin("Inspector");
    if (registry.valid(m_selectedEntity))
    {
        struct Row {
            int order;
            entt::id_type id;
            entt::meta_type type;
        };

        std::vector<Row> rows;
        for (auto&& [id, storage] : registry.storage())
        {
            if (!storage.contains(m_selectedEntity))
                continue;

            entt::meta_type type = entt::resolve(id);
            if (!type)
                continue;

            int order = type.invoke("order"_hs, {}).cast<int>();
            rows.push_back({ order, id, type });
        }

        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
            return a.order != b.order ? a.order < b.order : a.id < b.id;   // id tie-break keeps the order stable between frames
        });

        for (const Row& row : rows)
        {
            ImGui::PushID(static_cast<int>(row.id));

            entt::meta_any comp = row.type.invoke("get"_hs, {}, entt::forward_as_meta(registry), m_selectedEntity);
            row.type.invoke("inspect"_hs, comp);

            if (ImGui::BeginPopupContextItem("ComponentContextMenu"))
            {
                if (ImGui::MenuItem("Delete Component"))
                {
                    if (auto removeFunc = row.type.func("remove"_hs))
                        removeFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry), entt::forward_as_meta(m_selectedEntity));
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }
    }
    else
    {
		ImGui::Text("No entity selected.");
    }
    ImGui::End();
}

void SceneManager::Play()
{
	m_snapshot = SaveScene();
	Engine.SetEngineMode(EngineMode::Playing);
    Engine.GetECS()->GetRegistry().clear();

	LoadScene(m_snapshot);

    // Create a default sprite layer for the gizmos
    auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
    auto gizmoLayer = renderer.FindOrCreateSpriteLayer(-1);
    gizmoLayer->hiddenInInspector = true;
    gizmoLayer->generated = true;

	for (auto&& s : Engine.GetECS()->GetSystems<System>()) {
		if (s->GetRunMode() == RunMode::Playing || s->GetRunMode() == RunMode::Always) {
			s->OnPlayStart();
		}
	}
}

void SceneManager::Stop()
{
    for (auto&& s : Engine.GetECS()->GetSystems<System>()) {
		if (s->GetRunMode() == RunMode::Playing || s->GetRunMode() == RunMode::Always) {
			s->OnPlayStop();
		}
    }

    Engine.GetECS()->GetRegistry().clear();
	Engine.SetEngineMode(EngineMode::Editing);
    LoadScene(m_snapshot);
}

void SceneManager::DeleteSelectedEntity()
{
    if (m_selectedEntity == entt::null)
        return;

    auto* ecs = Engine.GetECS();
    auto& registry = ecs->GetRegistry();

    // Collect the entity and everything below it before deleting anything
    std::vector<entt::entity> stack{ m_selectedEntity }, doomed;
    while (!stack.empty()) {
        const entt::entity e = stack.back();
        stack.pop_back();
        doomed.push_back(e);
        for (entt::entity child : GetChildren(e))
            stack.push_back(child);
    }

    // Children first, going through ECS::DeleteEntity as before so any
    // cleanup it does for systems still runs for each of them
    for (auto it = doomed.rbegin(); it != doomed.rend(); ++it)
        ecs->DeleteEntity(*it);

    m_selectedEntity = entt::null;
    if (m_selectionGizmo)
        m_selectionGizmo->SetSelectedEntity(entt::null);
    if (m_translateGizmo)
        m_translateGizmo->SetSelectedEntity(entt::null);
}

void SceneManager::UpdateSelectedEntity(entt::entity entity)
{
    m_selectedEntity = entity;

	if (m_selectionGizmo)
        m_selectionGizmo->SetSelectedEntity(entity);
    if (m_translateGizmo)
		m_translateGizmo->SetSelectedEntity(entity);
}

json SceneManager::SaveScene() {
    auto ecs = Engine.GetECS();
    auto& registry = ecs->GetRegistry();
    json scene;
    scene["version"] = 1.0;

    auto systems = Engine.GetECS()->GetSystems<System>();
    for (auto* s : systems) {
        scene["systems"][s->title] = s->Save();
    }

    auto view = registry.view<Transform>();
    std::vector<entt::entity> entities(view.begin(), view.end());
    std::reverse(entities.begin(), entities.end());

    std::unordered_map<entt::entity, int> indexOf;
    for (size_t i = 0; i < entities.size(); ++i)
        indexOf[entities[i]] = static_cast<int>(i);

    scene["entities"] = json::array(); // also covers an empty scene (see the note at the end)
    json parents = json::array();

    for (auto e : entities) {
        json ent;
        for (auto& ser : GetSerializers()) {
            ser.save(registry, e, ent, ser.name);
        }
        scene["entities"].push_back(ent);

        const auto it = indexOf.find(registry.get<Transform>(e).parent);
        parents.push_back(it != indexOf.end() ? it->second : -1); // -1 = no parent
    }
    scene["parents"] = std::move(parents);

    return scene;
}

void SceneManager::LoadScene(const json& scene) {
	auto ecs = Engine.GetECS();
	ecs->GetRegistry().clear(); // Clear existing entities and components

    std::vector<entt::entity> created; // same order as the saved list

    for (const auto& ent : scene["entities"]) {
        auto entity = ecs->CreateEntity();
        created.push_back(entity);
        for (auto& ser : GetSerializers()) {
            ser.load(ecs->GetRegistry(), entity, ent, ser.name);
        }
    }

    if (scene.contains("parents") && scene["parents"].is_array()) {
        const auto& parents = scene["parents"];
        const size_t count = (std::min) (created.size(), parents.size());
        for (size_t i = 0; i < count; ++i) {
            if (!parents[i].is_number_integer())
                continue;
            const int p = parents[i].get<int>();
            if (p < 0 || p >= static_cast<int>(created.size()))
                continue;
            SetParent(created[i], created[p], false); // saved values are already local
        }
    }

	for (auto* s : ecs->GetSystems<System>()) {
		if (scene["systems"].contains(s->title)) {
			s->Load(scene["systems"][s->title]);
		}
	}
}

bool SceneManager::SaveSceneToFile(const std::filesystem::path& path) {
    if (Engine.GetEngineMode() == EngineMode::Playing) return false;

	json scene = SaveScene();
    std::filesystem::create_directories(path.parent_path());   // make assets/scenes/ if missing

    // Write to a temp file first, then rename, so a crash mid-write can't corrupt the old save
    std::filesystem::path tmp = path;
    tmp += ".tmp";

    {
        std::ofstream file(tmp);
        if (!file) return false;
        file << scene.dump(2);            // 2-space indent keeps it readable and diff-friendly
        if (!file) return false;
    }                                     // closed (flushed) here

    std::filesystem::rename(tmp, path);   // replaces the existing file on Windows with std::filesystem
    return true;
}

bool SceneManager::LoadSceneFromFile(const std::filesystem::path& path) {
    if (Engine.GetEngineMode() == EngineMode::Playing) return false;

    std::ifstream file(path);
    if (!file) return false;

	json scene = json::parse(file, nullptr, false);   // don't throw on parse errors
	if (scene.is_discarded()) return false;

	LoadScene(scene);

	m_loadedScenePath = path;

	return true;
}

void SceneManager::DrawContents()
{
    auto* ecs = Engine.GetECS();

    if (ImGui::Button("Add Entity")) { ImGui::OpenPopup("CreateEntityPopup"); }

    if (ImGui::BeginPopup("CreateEntityPopup")) {
        if (ImGui::MenuItem("Create New Entity")) {
            Entity entity = ecs->CreateEntity();
            ecs->CreateComponent<Transform>(entity);
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (m_selectedEntity != entt::null && ImGui::Button("Delete Entity"))
    {
        DeleteSelectedEntity();
    }

    ImGui::SameLine();
    ImGui::SeparatorText("Entities");

    ImGui::BeginChild("EntityList", ImVec2(0, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_Borders);

    auto& registry = ecs->GetRegistry();

    // One pass: which entities are roots, and who are each entity's children?
    ChildMap children;
    std::vector<entt::entity> roots;
    for (auto entity : registry.view<Transform>())
    {
        const entt::entity parent = registry.get<Transform>(entity).parent;
        if (parent != entt::null && registry.valid(parent) && registry.all_of<Transform>(parent))
            children[parent].push_back(entity);
        else
            roots.push_back(entity); // no parent, or a parent that no longer exists
    }

    bool deleteRequested = false;
    for (entt::entity root : roots)
        DrawEntityNode(root, children, deleteRequested);

    // Empty space under the list: drop an entity here to unparent it
    ImGui::InvisibleButton("##unparent", ImVec2(-1.0f, (std::max) (24.0f, ImGui::GetContentRegionAvail().y)));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
            clvr::SetParent(*static_cast<const entt::entity*>(payload->Data), entt::null);
        ImGui::EndDragDropTarget();
    }

    if (deleteRequested)
        DeleteSelectedEntity(); // safe now: nothing is iterating the tree

    ImGui::EndChild();
}

void SceneManager::OnOpen()
{}

void SceneManager::OnClose()
{}

void SceneManager::DrawEntityNode(entt::entity entity, const ChildMap& children, bool& deleteRequested)
{
    auto& registry = Engine.GetECS()->GetRegistry();

    const Transform& transform = registry.get<Transform>(entity);
    const std::string label = transform.name.empty()
        ? "Entity " + std::to_string(entt::to_integral(entity))
        : transform.name;

    const auto childIt = children.find(entity);
    const bool hasChildren = childIt != children.end();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (m_selectedEntity == entity)
        flags |= ImGuiTreeNodeFlags_Selected;

    // When scrolling to an entity, open its ancestors so it is actually visible
    if (hasChildren && m_scrollEntity != entt::null && m_scrollEntity != entity &&
        IsAncestorOrSelf(entity, m_scrollEntity))
        ImGui::SetNextItemOpen(true);

    ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
    const bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

    // The arrow toggles the node, clicking the label selects it
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        UpdateSelectedEntity(entity);

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(entity));
        ImGui::TextUnformatted(label.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY"))
            clvr::SetParent(*static_cast<const entt::entity*>(payload->Data), entity);
        ImGui::EndDragDropTarget();
    }

    if (m_scrollEntity != entt::null && m_scrollEntity == entity) {
        ImGui::SetScrollHereY();
        m_scrollEntity = entt::null;
    }

    if (ImGui::BeginPopupContextItem("EntityContextMenu")) {
        UpdateSelectedEntity(entity); // right-click selects this row too

        ImGui::Text("Entity: %s", label.c_str());
        ImGui::Separator();

        if (ImGui::BeginMenu("Add Component")) {
            bool noComponentsToAdd = true;
            for (auto&& [id, type] : entt::resolve()) {
                auto nameFunc = type.func("name"_hs);
                auto hasFunc = type.func("has"_hs);
                auto createFunc = type.func("create"_hs);

                if (!nameFunc || !hasFunc || !createFunc)
                    continue;

                const char* name = nameFunc.invoke(entt::meta_handle{}).cast<const char*>();

                bool has = hasFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry),
                                          entt::forward_as_meta(entity)).cast<bool>();

                noComponentsToAdd = has && noComponentsToAdd;

                if (!has && ImGui::MenuItem(name)) {
                    createFunc.invoke(entt::meta_handle{}, entt::forward_as_meta(registry), entt::forward_as_meta(entity));
                }
            }
            if (noComponentsToAdd)
                ImGui::Text("No components available to add.");
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Clear Parent"))
            clvr::SetParent(entity, entt::null);

        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
            deleteRequested = true; // selection was set above, so this deletes this entity

        ImGui::EndPopup();
    }

    if (open)
    {
        if (hasChildren)
            for (entt::entity child : childIt->second)
                DrawEntityNode(child, children, deleteRequested);
        ImGui::TreePop(); // required for every node that returned true, leaves included
    }

    ImGui::PopID();
}