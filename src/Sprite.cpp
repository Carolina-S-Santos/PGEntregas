#include "Sprite.h"

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

const float xOffset = 50.0f;

void Sprite::initialize(GLuint shaderID, GLuint texID, int nAnimations, int nFrames, vec3 pos, vec3 dimensions, float angle)
{
	this->shaderID = shaderID;
	this->texID = texID;
	this->pos = pos;
	this->dimensions = dimensions;
	this->angle = angle;
	this->nAnimations = nAnimations;
	this->nFrames = nFrames;
	this->iAnimations = 4;
	this->iFrames = 0;
	this->d.s = 1.0 / (float)nFrames;
	this->d.t = 1.0 / (float)nAnimations;
	this->FPS = 12.0;
	this->lastTime = 0.0;
	this->vel = 7.5;
	this->VAO = setupGeometry();
}

void Sprite::update()
{
	mat4 model = mat4(1);
	model = translate(model, pos);
	model = scale(model, dimensions);
	// Mandar a matriz de modelo para o shader
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	vec2 offsetTex = vec2(iFrames * d.s, iAnimations * d.t);
	glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

	float now = glfwGetTime();
	float dt = now - lastTime;

	if (dt >= 1.0 / FPS)
	{
		iFrames = (iFrames + 1) % nFrames;
		lastTime = now;
	}
}

void Sprite::draw()
{
	glBindVertexArray(VAO);				 // Conectando ao buffer de geometria
	glBindTexture(GL_TEXTURE_2D, texID); // Conectando ao buffer de textura
	// Chamada de desenho - drawcall
	// Poligono Preenchido - GL_TRIANGLE_STRIP
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
}

void Sprite::moveRight(float deltaTime)
{
	float halfWidth = dimensions.x / 2.0f;
	float rightLimit = 800.0f - halfWidth;
	float movement = vel * 60.0f * deltaTime; // Normaliza a velocidade para 60 FPS
	if (pos.x + movement <= rightLimit + xOffset)
	{
		pos.x += movement;
	}
	else
	{
		pos.x = rightLimit + xOffset;
	}
	iAnimations = 4;
}

void Sprite::moveLeft(float deltaTime)
{
	float halfWidth = dimensions.x / 2.0f;
	float leftLimit = halfWidth;
	float movement = vel * 60.0f * deltaTime; // Normaliza a velocidade para 60 FPS
	if (pos.x - movement >= leftLimit - xOffset)
	{
		pos.x -= movement;
	}
	else
	{
		pos.x = leftLimit - xOffset;
	}
	iAnimations = 3;
}

GLuint Sprite::setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {
		// x   y     z    s   t
		-0.5, 0.5, 0.0, 0.0, d.t,  // v0
		-0.5, -0.5, 0.0, 0.0, 0.0, // v1
		0.5, 0.5, 0.0, d.s, d.t,   // v2
		0.5, -0.5, 0.0, d.s, 0.0   // v3
	};

	GLuint VBO;
	// Geração do identificador do VBO
	glGenBuffers(1, &VBO);
	// Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);
	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos
	glBindVertexArray(VAO);
	// Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
	//  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	//  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
	//  Tipo do dado
	//  Se está normalizado (entre zero e um)
	//  Tamanho em bytes
	//  Deslocamento a partir do byte zero

	// Atributo posição - x, y, z
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	// Atributo coordenada de textura - s, t
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}
