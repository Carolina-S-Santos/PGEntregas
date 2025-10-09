#include <iostream>
#include <string>
#include <assert.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>

using namespace std;

// --- GLAD ---------------------------------------------------------------
// GLAD é usado para carregar os ponteiros das funções OpenGL em tempo de
// execução. Deve ser inicializado (gladLoadGLLoader) depois que o contexto
// OpenGL foi criado (glfwMakeContextCurrent).
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Sprite.h"
#include "Texture.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipo da função de callback de mouse
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

// Protótipos das funções
int setupShader();
void processInput(Sprite &spr, float deltaTime);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// === Shaders embutidos (GLSL) ==========================================
// Vertex shader: transforma posições usando 'projection' e 'model' e
// passa coordenadas de textura para o fragment shader.
const GLchar *vertexShaderSource = R"glsl(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 out vec2 tex_coord; 
 uniform mat4 projection;
 uniform mat4 model;
 void main()
 {
	 gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	 tex_coord = vec2(texc.s,1.0-texc.t);
 }
 )glsl";

// Fragment shader: amostra a textura usando 'tex_coord' e um 'offsetTex'
// opcional. Mantemos este shader simples para renderizar sprites.
const GLchar *fragmentShaderSource = R"glsl(
 #version 400
 in vec2 tex_coord;
 out vec4 color;

uniform sampler2D tex_buffer;
uniform vec2 offsetTex;

 void main()
 {
	 color = texture(tex_buffer,tex_coord+offsetTex);
 }
 )glsl";

// === Variáveis globais de estado do jogo =================================
// 'keys' guarda o estado das teclas (true quando pressionada)
bool keys[1024];

// Controle de efeito visual (explosão) que aparece ao coletar tomate
bool showExplosion = false;
double explosionTimer = 0.0;

// Configurações e contadores do jogo
const int NUM_TOMATOES = 2; // número de tomates simultâneos
int tomatoCount = 0;		// tomates coletados
int missedTomatoes = 0;		// tomates que caíram no chão
const int MAX_MISSED = 5;	// limite para game over
bool gameOver = false;		// flag indicando fim de jogo

// Sprites e ponteiros globais necessários em callbacks
Sprite *g_sheep = nullptr;		 // ponteiro para sprite do jogador
Sprite *g_explosion = nullptr;	 // ponteiro para sprite de explosão
GLFWwindow *g_window = nullptr;	 // ponteiro para janela (usado em callbacks)
Sprite gameOverSprite;			 // sprite da tela de game over
Sprite restartSprite;			 // sprite do botão restart
Sprite exitSprite;				 // sprite do botão exit
Sprite tomatoes[NUM_TOMATOES];	 // array de sprites dos tomates
bool tomatoActive[NUM_TOMATOES]; // se cada tomate está ativo

int main()
{
	// ---------------- Inicialização -------------------------------------
	// Seed do gerador de números aleatórios (usado para posições dos tomates)
	srand(static_cast<unsigned int>(time(nullptr)));

	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Tomates Pegos: 0 | Perdidos: 0/5", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	g_window = window; // Store global reference

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Inicializar o array de controle de teclas
	for (int i = 0; i < 1024; i++)
	{
		keys[i] = false;
	}

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();
	GLuint texID = loadTexture("./assets/sprites/sheep.png");
	GLuint bgTexID = loadTexture("./assets/tex/Summer2.png");
	GLuint explosionTexID = loadTexture("./assets/sprites/tomatoExplosion.png");
	GLuint tomatoTexID = loadTexture("./assets/sprites/tomato.png");
	GLuint gameOverTexID = loadTexture("./assets/text/GAME OVER.png");
	GLuint restartTexID = loadTexture("./assets/text/RESTART.png");
	GLuint exitTexID = loadTexture("./assets/text/EXIT.png");

	Sprite spr;
	spr.initialize(shaderID, texID, 8, 6, vec3(400.0, 70.0, 0.5), vec3(64.0 * 3, 64.0 * 3, 1.0));

	// Background sprite
	Sprite background;
	background.initialize(shaderID, bgTexID, 1, 1, vec3(400.0, 300.0, -0.5), vec3(800.0, 600.0, 1.0));

	// Explosion sprite (above sheep)
	Sprite explosion;
	explosion.initialize(shaderID, explosionTexID, 1, 1, vec3(400.0, 70.0, 1.0), vec3(64.0, 64.0, 1.0));

	// Tomato sprites (multiple falling from top)
	for (int i = 0; i < NUM_TOMATOES; i++)
	{
		float xPos = 100.0 + (rand() % 600);
		float yPos = 550.0 + (i * 150.0); // Stagger vertical positions
		tomatoes[i].initialize(shaderID, tomatoTexID, 1, 1, vec3(xPos, yPos, 0.6), vec3(64.0, 64.0, 1.0));
		tomatoActive[i] = true;
	}

	// Initialize Game Over screen sprites mantendo as proporções originais
	float gameOverScale = 0.25f; // Escala para o sprite GAME OVER
	float buttonScale = 0.12f;	 // Escala para os botões

	// GAME OVER: 1024x692 original
	gameOverSprite.initialize(shaderID, gameOverTexID, 1, 1,
							  vec3(WIDTH / 2, HEIGHT / 2 + 40, 0.7),
							  vec3(1024.0f * gameOverScale, 692.0f * gameOverScale, 1.0));

	// RESTART: 1246x308 original
	restartSprite.initialize(shaderID, restartTexID, 1, 1,
							 vec3(WIDTH / 2, HEIGHT / 2 - 80, 0.7),
							 vec3(1246.0f * buttonScale, 308.0f * buttonScale, 1.0));

	// EXIT: 1024x438 original
	exitSprite.initialize(shaderID, exitTexID, 1, 1,
						  vec3(WIDTH / 2, HEIGHT / 2 - 140, 0.7),
						  vec3(1024.0f * buttonScale, 438.0f * buttonScale, 1.0));

	// Set global pointers for callbacks
	g_sheep = &spr;
	g_explosion = &explosion;

	// Habilitação do teste de profundidade
	glEnable(GL_DEPTH_TEST);

	// Habilitação da transparência
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros

	double prev_s = glfwGetTime();	// Define o "tempo anterior" inicial.
	double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.

	// Define o FPS desejado
	const double targetFPS = 60.0;
	const double targetFrameTime = 1.0 / targetFPS;

	// Criação da matriz de projeção
	mat4 projection = ortho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);

	// Utilizamos a variáveis do tipo uniform em GLSL para armazenar esse tipo de info
	// que não está nos buffers
	// Mandar a matriz de projeção para o shader
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	mat4 model = mat4(1); // matriz identidade

	// Ativar o primeiro buffer de textura do OpenGL
	glActiveTexture(GL_TEXTURE0);

	// Registrando o nome que o buffer da textura terá no fragment shader
	glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Calcula o tempo decorrido para animações e física
		double curr_s = glfwGetTime();		// Obtém o tempo atual.
		double elapsed_s = curr_s - prev_s; // Calcula o tempo decorrido desde o último frame.

		// Se o frame atual foi processado muito rapidamente, espera um pouco
		if (elapsed_s < targetFrameTime)
		{
			double sleepTime = targetFrameTime - elapsed_s;
			glfwWaitEventsTimeout(sleepTime);
			curr_s = glfwGetTime();
			elapsed_s = curr_s - prev_s;
		}

		prev_s = curr_s; // Atualiza o "tempo anterior" para o próximo frame.

		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		if (!gameOver)
		{
			processInput(spr, elapsed_s);
		}

		// Update explosion timer
		if (showExplosion && explosionTimer > 0.0)
		{
			explosionTimer -= elapsed_s;
			if (explosionTimer <= 0.0)
			{
				showExplosion = false;
			}
		}

		// Update tomato positions (falling) - only if game is not over
		if (!gameOver)
		{
			vec3 sheepPos = spr.getPos();
			float tomatoHalfWidth = 32.0; // half of 64.0
			float tomatoHalfHeight = 32.0;
			float sheepHalfWidth = 48.0;  // Reduzido para melhor ajuste com a parte visível
			float sheepHalfHeight = 48.0; // Reduzido para melhor ajuste com a parte visível

			for (int i = 0; i < NUM_TOMATOES; i++)
			{
				if (tomatoActive[i])
				{
					vec3 tomatoPos = tomatoes[i].getPos();
					tomatoPos.y -= 200.0 * elapsed_s; // Fall speed: 200 pixels per second
					tomatoes[i].setPos(tomatoPos);

					// Check collision with sheep
					// Simple AABB collision detection
					bool collisionX = abs(tomatoPos.x - sheepPos.x) < (tomatoHalfWidth + sheepHalfWidth);
					bool collisionY = abs(tomatoPos.y - sheepPos.y) < (tomatoHalfHeight + sheepHalfHeight);
					if (collisionX && collisionY)
					{
						// Collision detected! Show explosion when tomato hits the sheep
						showExplosion = true;
						explosionTimer = 0.2; // 200ms

						// Increment tomato counter
						tomatoCount++;

						// Update window title with counter
						char title[256];
						sprintf(title, "Tomates Pegos: %d | Perdidos: %d/%d", tomatoCount, missedTomatoes, MAX_MISSED);
						glfwSetWindowTitle(window, title);

						// Position explosion at the collision point, synchronized with tomato collision
						explosion.setPos(vec3(tomatoPos.x, tomatoPos.y, 1.0));

						// Reset this tomato to the top with a new random position
						tomatoPos.y = 550.0 + (rand() % 200); // Random height to stagger
						tomatoPos.x = 100.0 + (rand() % 600); // Random X position
						tomatoes[i].setPos(tomatoPos);
					}

					// Check if tomato fell off screen (missed)
					if (tomatoPos.y < -50.0)
					{
						// Increment missed counter
						missedTomatoes++;

						// Update window title
						char title[256];
						sprintf(title, "Tomates Pegos: %d | Perdidos: %d/%d", tomatoCount, missedTomatoes, MAX_MISSED);
						glfwSetWindowTitle(window, title);

						// Check for game over
						if (missedTomatoes >= MAX_MISSED)
						{
							gameOver = true;
							sprintf(title, "GAME OVER! Tomates Pegos: %d | Perdidos: %d", tomatoCount, missedTomatoes);
							glfwSetWindowTitle(window, title);
						}
						else
						{
							// Reset tomato to top
							tomatoPos.y = 550.0 + (rand() % 200);
							tomatoPos.x = 100.0 + (rand() % 600); // Random X position
							tomatoes[i].setPos(tomatoPos);
						}
					}
				}
			}
		}

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// Desenhar o background primeiro (atrás de tudo)
		background.update();
		background.draw();

		// Atualizar e desenhar o sprite do jogador (ovelha)
		spr.update();
		spr.draw();

		// Desenhar todos os tomates ativos (apenas se o jogo não terminou)
		if (!gameOver)
		{
			for (int i = 0; i < NUM_TOMATOES; i++)
			{
				if (tomatoActive[i])
				{
					tomatoes[i].update();
					tomatoes[i].draw();
				}
			}
		}

		// Desenhar explosão se ativa
		if (showExplosion)
		{
			explosion.update();
			explosion.draw();
		}

		// Se estiver em estado de game over, desenha a tela de opções
		if (gameOver)
		{
			gameOverSprite.update();
			gameOverSprite.draw();
			restartSprite.update();
			restartSprite.draw();
			exitSprite.update();
			exitSprite.draw();
		}

		glBindVertexArray(0); // limpa VAO atual (não estritamente necessário aqui)

		// Troca os buffers da janela (double buffering)
		glfwSwapBuffers(window);
	}

	// Pede pra OpenGL desalocar os buffers (se houver) e finaliza GLFW
	// glDeleteVertexArrays(1, &VAO); // exemplo se houver VAO(s)
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (action == GLFW_PRESS)
	{
		keys[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		keys[key] = false;
	}
}

// Esta função está bastante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void processInput(Sprite &spr, float deltaTime)
{
	if (keys[GLFW_KEY_D])
	{
		spr.moveRight(deltaTime);
	}
	if (keys[GLFW_KEY_A])
	{
		spr.moveLeft(deltaTime);
	}
}

// Função de callback de mouse - É chamada sempre que um botão do mouse for pressionado
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (gameOver && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		ypos = HEIGHT - ypos; // Inverter Y pois OpenGL tem origem no canto inferior esquerdo

		// Verificar clique no botão RESTART (usando as dimensões corretas 1246x308)
		vec3 restartPos = restartSprite.getPos();
		float restartWidth = 1246.0f * 0.12f * 0.5f; // Metade da largura
		float restartHeight = 308.0f * 0.12f * 0.5f; // Metade da altura
		if (xpos >= restartPos.x - restartWidth && xpos <= restartPos.x + restartWidth &&
			ypos >= restartPos.y - restartHeight && ypos <= restartPos.y + restartHeight)
		{
			// Reiniciar o jogo
			gameOver = false;
			tomatoCount = 0;
			missedTomatoes = 0;

			// Reposicionar os tomates
			for (int i = 0; i < NUM_TOMATOES; i++)
			{
				float xPos = 100.0 + (rand() % 600);
				float yPos = 550.0 + (i * 150.0); // Separar verticalmente
				tomatoes[i].setPos(vec3(xPos, yPos, 0.6));
				tomatoActive[i] = true;
			}

			// Reset window title
			char title[256];
			sprintf(title, "Tomates Pegos: %d | Perdidos: %d/%d", tomatoCount, missedTomatoes, MAX_MISSED);
			glfwSetWindowTitle(window, title);
		}
		// Verificar clique no botão EXIT (usando as dimensões corretas 1024x438)
		vec3 exitPos = exitSprite.getPos();
		float exitWidth = 1024.0f * 0.12f * 0.5f; // Metade da largura
		float exitHeight = 438.0f * 0.12f * 0.5f; // Metade da altura
		if (xpos >= exitPos.x - exitWidth && xpos <= exitPos.x + exitWidth &&
			ypos >= exitPos.y - exitHeight && ypos <= exitPos.y + exitHeight)
		{
			// Fechar o jogo
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
}
