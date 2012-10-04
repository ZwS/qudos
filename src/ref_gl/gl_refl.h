/*******************************************************
	gl_refl.h

	by:
		Matt Ownby	- original code
		dukey		- rewrote most of the above :)
		spike		- helped me with the matrix maths
		jitspoe		- shaders


	adds reflective water to the Quake2 engine

*******************************************************/

void		R_init_refl(int maxNoReflections);
void		R_shutdown_refl(void);
void		R_setupArrays(int maxNoReflections);
void		R_clear_refl(void);
void		R_add_refl(float x, float y, float z, float normalX, float normalY, float normalZ, float distance2);
int		txm_genTexObject(unsigned char *texData, int w, int h,
                                 int format, qboolean repeat, qboolean mipmap);
void		R_RecursiveFindRefl(mnode_t * node);
void		R_DrawDebugReflTexture();
void		R_UpdateReflTex(refdef_t * fd);
void		R_DoReflTransform(qboolean update);
void		R_LoadReflMatrix();
void		R_ClearReflMatrix();
void		setupClippingPlanes();

/* vars other files need access to */
extern qboolean	g_drawing_refl;
extern qboolean	g_refl_enabled;
extern unsigned int g_reflTexW, g_reflTexH;
extern float	g_refl_aspect;
extern float   *g_refl_X;
extern float   *g_refl_Y;
extern float   *g_refl_Z;
extern float   *g_waterDistance2;
extern int     *g_tex_num;
extern int	g_active_refl;
extern int	g_num_refl;
extern vec3_t  *waterNormals;


extern unsigned int gWaterProgramId;
extern image_t *distortTex;
extern image_t *waterNormalTex;
extern qboolean	brightenTexture;
