#pragma once

// The real SDK exposes this flag as a bool and the manager as a reference.
class CMenuManager {
public:
    bool m_bMenuActive = false;
};
inline CMenuManager TestFrontEndMenuManager;
inline CMenuManager& FrontEndMenuManager = TestFrontEndMenuManager;
