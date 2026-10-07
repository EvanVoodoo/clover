#include "input/input_handler.hpp"

using namespace clvr;

InputHandler::InputHandler()
{}

InputHandler::~InputHandler()
{}

void InputHandler::Update(float dt) {}

void InputHandler::Render() {}

void InputHandler::Inspect(float dt) {}

json InputHandler::Save() { return json(); }

void InputHandler::Load(const json&) {}
