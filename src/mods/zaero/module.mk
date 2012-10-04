zaero_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/zaero/*.c)
ifeq ($(strip $(WITH_BOTS)),YES)
zaero_SRCS+=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/zaero/ace/*.c)
endif
zaero_OBJS=	$(patsubst %,$(OBJ_DIR)/zaero/%,$(notdir $(zaero_SRCS:%.c=%.o)))

$(OBJ_DIR)/zaero/.depend: $(zaero_SRCS)
	@mkdir -p $(OBJ_DIR)/zaero $(BIN_DIR)/zaero
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/zaero/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/zaero/$(GAME_NAME): $(zaero_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
