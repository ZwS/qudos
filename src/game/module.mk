game_SRCS=	$(wildcard $(SRC_DIR)/game/*.c)
ifeq ($(strip $(WITH_BOTS)),YES)
game_SRCS+=	$(wildcard $(SRC_DIR)/game/ace/*.c)
endif
game_OBJS=	$(patsubst %,$(OBJ_DIR)/game/%,$(notdir $(game_SRCS:%.c=%.o)))

$(OBJ_DIR)/game/.depend: $(game_SRCS)
	@mkdir -p $(OBJ_DIR)/game $(BIN_DIR)/baseq2
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/game/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/game/$(GAME_NAME): $(BIN_DIR)/baseq2/$(GAME_NAME)

$(BIN_DIR)/baseq2/$(GAME_NAME): $(game_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
