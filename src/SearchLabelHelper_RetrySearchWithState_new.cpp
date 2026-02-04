bool RetrySearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	// Check for null pointers before proceeding
	if (!page || !parentDlg) {
		return false;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return false;
	}

	// Find page index
	int pageIndex = -1;
	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == page) {
			pageIndex = i;
			break;
		}
	}

	// Check if page was found in notebook
	if (pageIndex == -1) {
		return false;
	}

	// Get the search ID and tab text
	long searchId = page->GetSearchId();
	assert(searchId > 0);

	wxString tabText = notebook->GetPageText(pageIndex);
	assert(!tabText.IsEmpty());

	// Determine search type from tab text
	bool isKadSearch = tabText.StartsWith(wxT("[Kad]")) || tabText.StartsWith(wxT("!"));
	bool isEd2kSearch = tabText.StartsWith(wxT("[Local] ")) || tabText.StartsWith(wxT("[Global] "));

	// Only retry ED2K searches (Local/Global), not Kad searches
	if (!isEd2kSearch) {
		return false;
	}

	// Check retry limit (max 3 retries)
	const int MAX_RETRIES = 3;
	int retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	if (retryCount >= MAX_RETRIES) {
		// Maximum retries reached - show final state
		UpdateSearchState(page, parentDlg, wxT("No Results"));
		return false;
	}

	// Start retry - this increments the retry counter
	if (!parentDlg->GetStateManager().RequestRetry(searchId)) {
		// Failed to start retry
		return false;
	}

	// Update state to show retry is in progress with count
	retryCount = parentDlg->GetStateManager().GetRetryCount(searchId);
	wxString retryState = CFormat(wxT("Retrying %d")) % retryCount;
	UpdateSearchState(page, parentDlg, retryState);

	// Get the search parameters for this search
	// clang-format off
	CSearchList::CSearchParams params;
	// clang-format on
	if (!parentDlg->GetStateManager().GetSearchParams(searchId, params)) {
		// No search parameters available - cannot retry
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Start a new ED2K search with same parameters
	// Use same search ID to keep results in same tab
	// Determine if this is a Local or Global search
	search::ModernSearchType modernSearchType = search::ModernSearchType::LocalSearch;
	if (tabText.StartsWith(wxT("[Global] "))) {
		modernSearchType = search::ModernSearchType::GlobalSearch;
	}

	// Convert legacy params to modern params
	search::SearchParams modernParams;
	modernParams.searchString = params.searchString;
	modernParams.strKeyword = params.strKeyword;
	modernParams.typeText = params.typeText;
	modernParams.extension = params.extension;
	modernParams.minSize = params.minSize;
	modernParams.maxSize = params.maxSize;
	modernParams.availability = params.availability;
	modernParams.searchType = modernSearchType;

	// Create SearchController using factory
	auto controller = search::SearchControllerFactory::createController(modernSearchType);
	if (!controller) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Set up callbacks for controller
	parentDlg->SetupControllerCallbacks(controller.get());

	// Start search
	controller->startSearch(modernParams);

	// Get search ID after starting
	uint32_t newSearchId = controller->getSearchId();
	if (newSearchId == 0) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Store controller
	parentDlg->m_searchControllers[newSearchId] = std::move(controller);

	// Update the page to show results from the new search
	page->ShowResults(newSearchId);

	// Retry initiated successfully - state will be updated to "Searching"
	// when search starts
	return true;
}
