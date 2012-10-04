rogue_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/rogue/*.c)
ifeq ($(strip $(WITH_BOTS)),YES)
rogue_SRCS+=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/rogue/ace/*.c)
endif
rogue_OBJS=	$(patsubst %,$(OBJ_DIR)/rogue/%,$(notdir $(rogue_SRCS:%.c=%.o)))

$(OBJ_DIR)/rogue/.depend: $(rogue_SRCS)
	@mkdir -p $(OBJ_DIR)/rogue $(BIN_DIR)/rogue
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/rogue/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/rogue/$(GAME_NAME): $(rogue_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
