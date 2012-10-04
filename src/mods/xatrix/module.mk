xatrix_SRCS=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/xatrix/*.c)
ifeq ($(strip $(WITH_BOTS)),YES)
xatrix_SRCS+=	$(wildcard $(SRC_DIR)/$(MOD_DIR)/xatrix/ace/*.c)
endif
xatrix_OBJS=	$(patsubst %,$(OBJ_DIR)/xatrix/%,$(notdir $(xatrix_SRCS:%.c=%.o)))

$(OBJ_DIR)/xatrix/.depend: $(xatrix_SRCS)
	@mkdir -p $(OBJ_DIR)/xatrix $(BIN_DIR)/xatrix
	$(CC) -MM $(CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/xatrix/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(CFLAGS) $$(SHLIB_CFLAGS) -o $$@ $$<' > $@

$(BIN_DIR)/xatrix/$(GAME_NAME): $(xatrix_OBJS)
	$(CC) -o $@ $^ $(SHLIB_LDFLAGS) $(LDFLAGS)
