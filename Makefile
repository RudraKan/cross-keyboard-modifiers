NAME   := cross-keyboard-modifiers
LABEL  := local.$(NAME)
BIN    := $(HOME)/.local/bin/$(NAME)
PLIST  := $(HOME)/Library/LaunchAgents/$(LABEL).plist
DOMAIN := gui/$(shell id -u)

$(NAME): $(NAME).c
	clang -O2 -Wall -Wextra -o $@ $< -framework ApplicationServices

install: $(NAME)
	-launchctl bootout $(DOMAIN)/$(LABEL) 2>/dev/null
	mkdir -p $(dir $(BIN)) $(dir $(PLIST))
	rm -f $(BIN) && cp $(NAME) $(BIN)
	sed -e 's|@LABEL@|$(LABEL)|g' -e 's|@BIN@|$(BIN)|g' launchagent.plist > $(PLIST)
	launchctl bootstrap $(DOMAIN) $(PLIST)
	@echo "Installed. Allow '$(NAME)' in System Settings > Privacy & Security > Accessibility."

uninstall:
	-launchctl bootout $(DOMAIN)/$(LABEL) 2>/dev/null
	rm -f $(BIN) $(PLIST)

clean:
	rm -f $(NAME)

.PHONY: install uninstall clean
