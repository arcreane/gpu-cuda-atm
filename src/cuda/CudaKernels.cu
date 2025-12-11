/**
 * @file CudaKernels.cu
 * @brief CUDA kernels pour la simulation physique des particules sur GPU
 *
 * Ce fichier contient tous les kernels CUDA utilisés pour calculer la physique
 * des particules en parallèle. Chaque kernel est optimisé pour le parallélisme GPU.
 */

#include "CudaKernels.cuh"
#include "SimulatorConfig.h"

/**
 * @brief Applique la gravité et le frottement sur toutes les particules
 *
 * Ce kernel met à jour les vitesses des particules en appliquant :
 * - La force de gravité (accélération vers le bas)
 * - Le frottement visqueux (ralentissement progressif)
 *
 * @param vx Tableau des vitesses en X (sur GPU)
 * @param vy Tableau des vitesses en Y (sur GPU)
 * @param gravity Coefficient de gravité (ex: 0.5)
 * @param friction Coefficient de frottement [0.0-1.0]
 * @param count Nombre total de particules
 * @param dt Delta time (temps écoulé depuis la dernière frame)
 *
 * Complexité: O(N) - Chaque thread traite une particule
 * Parallélisme: Un thread par particule
 */
__global__ void applyForcesKernel(float* vx, float* vy, float gravity, float friction, int count, float dt) {
    // Calcul de l'index global du thread (identifiant unique de particule)
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Vérification des limites : threads en trop ne font rien
    if (i >= count) return;

    // Applique la gravité : accélération en Y (vers le bas)
    vy[i] += gravity * dt;

    // Applique le frottement visqueux : réduction proportionnelle de la vitesse
    // Plus friction est élevé, plus les particules ralentissent rapidement
    float frictionFactor = 1.0f - friction;
    vx[i] *= frictionFactor;
    vy[i] *= frictionFactor;
}

/**
 * @brief Applique la force de la souris sur les particules proches
 *
 * Ce kernel gère l'interaction utilisateur avec la simulation via la souris :
 * - Clic gauche : Attraction des particules vers le curseur
 * - Clic droit : Répulsion (explosion) depuis le curseur
 * - Mouvement : Force proportionnelle à la vitesse du curseur (balayage)
 *
 * La force diminue avec la distance (inverse du rayon d'action)
 *
 * @param x, y Positions des particules
 * @param vx, vy Vitesses des particules
 * @param mouseX, mouseY Position du curseur
 * @param mouseVX, mouseVY Vitesse du curseur
 * @param forceRadius Rayon d'action de la souris
 * @param forceStrength Intensité de la force
 * @param leftButton Vrai si bouton gauche enfoncé
 * @param rightButton Vrai si bouton droit enfoncé
 * @param count Nombre de particules
 * @param dt Delta time
 */
__global__ void applyMouseForceKernel(float* x, float* y, float* vx, float* vy,
                                      float mouseX, float mouseY, float mouseVX, float mouseVY,
                                      float forceRadius, float forceStrength,
                                      bool leftButton, bool rightButton,
                                      int count, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    // Calcul de la distance particule-souris
    float dx = x[i] - mouseX;
    float dy = y[i] - mouseY;
    float distSq = dx * dx + dy * dy;
    float radiusSq = forceRadius * forceRadius;

    // Ne traiter que les particules dans le rayon d'action
    if (distSq < radiusSq && distSq > 0.001f) {
        float dist = sqrtf(distSq);

        // Force diminue avec la distance (1.0 au centre, 0.0 au bord)
        float strength = (1.0f - dist / forceRadius);

        // Vecteur normalisé particule → souris
        float nx = dx / dist;
        float ny = dy / dist;

        float forceMagnitude = forceStrength * strength * dt;

        if (leftButton) {
            // Mode attraction : tire les particules vers le curseur
            vx[i] -= nx * forceMagnitude;
            vy[i] -= ny * forceMagnitude;
        } else if (rightButton) {
            // Mode répulsion : pousse les particules loin du curseur (2× plus fort)
            vx[i] += nx * forceMagnitude * 2.0f;
            vy[i] += ny * forceMagnitude * 2.0f;
        } else {
            // Mode balayage : force proportionnelle à la vitesse du curseur
            float mouseSpeed = sqrtf(mouseVX * mouseVX + mouseVY * mouseVY);
            if (mouseSpeed > 1.0f) {
                vx[i] += mouseVX * strength * 0.5f;
                vy[i] += mouseVY * strength * 0.5f;
            }
        }
    }
}

/**
 * @brief Met à jour les positions des particules basées sur leurs vitesses
 *
 * Intégration d'Euler simple : position += vitesse × temps
 * Ce kernel doit être appelé après applyForcesKernel pour refléter
 * les changements de vitesse dans les positions.
 *
 * @param x, y Positions des particules (modifiées)
 * @param vx, vy Vitesses des particules
 * @param count Nombre de particules
 * @param dt Delta time
 *
 * Parallélisme: Un thread par particule, pas de dépendances
 */
__global__ void updatePositionsKernel(float* x, float* y, float* vx, float* vy, int count, float dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    // Intégration d'Euler : nouvelle_position = ancienne_position + vitesse * dt
    x[i] += vx[i] * dt;
    y[i] += vy[i] * dt;
}

/**
 * @brief Gère les collisions avec les bords de la fenêtre (murs)
 *
 * Détecte si une particule sort des limites et la fait rebondir :
 * 1. Repositionne la particule juste à l'intérieur de la limite
 * 2. Inverse la vitesse perpendiculaire au mur
 * 3. Applique le coefficient d'élasticité (perte d'énergie)
 *
 * @param x, y Positions des particules
 * @param vx, vy Vitesses des particules
 * @param radius Rayons des particules
 * @param width, height Dimensions de la zone de simulation
 * @param elasticity Coefficient de rebond [0.0=inélastique, 1.0=élastique parfait]
 * @param count Nombre de particules
 *
 * Exemple: elasticity=0.9 → la particule perd 10% de son énergie à chaque rebond
 */
__global__ void handleWallCollisionsKernel(float* x, float* y, float* vx, float* vy, float* radius,
                                           float width, float height, float elasticity, int count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    float r = radius[i];

    // Mur gauche : si le bord gauche de la particule sort
    if (x[i] - r < 0) {
        x[i] = r;  // Repositionne au bord
        vx[i] = -vx[i] * elasticity;  // Inverse la vitesse X avec perte d'énergie
    }
    // Mur droit
    if (x[i] + r > width) {
        x[i] = width - r;
        vx[i] = -vx[i] * elasticity;
    }
    // Mur haut
    if (y[i] - r < 0) {
        y[i] = r;
        vy[i] = -vy[i] * elasticity;
    }
    // Mur bas
    if (y[i] + r > height) {
        y[i] = height - r;
        vy[i] = -vy[i] * elasticity;
    }
}

/**
 * @brief Détecte et résout les collisions entre particules (algorithme optimisé par grille spatiale)
 *
 * ALGORITHME :
 * 1. Pour chaque particule, détermine sa cellule dans la grille
 * 2. Teste les collisions uniquement avec les particules des 9 cellules voisines (3×3)
 * 3. Si collision détectée :
 *    a) Sépare les particules proportionnellement à leurs masses
 *    b) Calcule et applique l'impulsion de collision (conservation de la quantité de mouvement)
 *
 * OPTIMISATION :
 * - Sans grille : O(N²) → 10,000 particules = 50 millions de tests
 * - Avec grille : O(N) → 10,000 particules ≈ 90,000 tests (550× plus rapide!)
 *
 * PHYSIQUE :
 * - Séparation proportionnelle aux masses inverses (particule lourde bouge moins)
 * - Impulsion basée sur la vitesse relative projetée sur la normale de collision
 * - Conservation parfaite de la quantité de mouvement et énergie (si elasticity=1.0)
 *
 * @param x, y Positions des particules
 * @param vx, vy Vitesses des particules
 * @param radius Rayons des particules
 * @param mass Masses des particules
 * @param cellStart Index de début de chaque cellule dans particleIndex
 * @param cellEnd Index de fin de chaque cellule
 * @param particleIndex Indices des particules triées par cellule
 * @param gridWidth, gridHeight Dimensions de la grille spatiale
 * @param cellSize Taille d'une cellule (recommandé : 2 × rayon_max)
 * @param elasticity Coefficient de restitution [0.0-1.0]
 * @param count Nombre total de particules
 * @param collisionCount Compteur de collisions (incrémenté atomiquement)
 *
 * ATTENTION : Utilise atomicAdd pour éviter les race conditions
 * (plusieurs threads peuvent modifier la même particule simultanément)
 */
__global__ void handleParticleCollisionsKernel(float* x, float* y, float* vx, float* vy,
                                               float* radius, float* mass,
                                               int* cellStart, int* cellEnd, int* particleIndex,
                                               int gridWidth, int gridHeight, float cellSize,
                                               float elasticity, int count, int* collisionCount) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;

    // Récupère l'index réel de la particule (peut être différent de idx après tri)
    int i = particleIndex[idx];

    // Charge les propriétés de la particule i dans des registres (plus rapide)
    float xi = x[i];
    float yi = y[i];
    float ri = radius[i];
    float mi = mass[i];
    float vxi = vx[i];
    float vyi = vy[i];

    // Détermine la cellule de la particule i
    int cellX = min(max(int(xi / cellSize), 0), gridWidth - 1);
    int cellY = min(max(int(yi / cellSize), 0), gridHeight - 1);

    // Parcourt les 9 cellules voisines (3×3 autour de la particule)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = cellX + dx;
            int ny = cellY + dy;

            // Vérifie que la cellule voisine est dans les limites de la grille
            if (nx < 0 || nx >= gridWidth || ny < 0 || ny >= gridHeight)
                continue;

            // Calcule l'index linéaire de la cellule voisine
            int cellIdx = ny * gridWidth + nx;
            int start = cellStart[cellIdx];
            int end = cellEnd[cellIdx];

            // Cellule vide : passe à la suivante
            if (start == -1) continue;

            // Teste toutes les particules de cette cellule
            for (int k = start; k < end; ++k) {
                int j = particleIndex[k];

                // Évite de tester deux fois la même paire (i,j) et (j,i)
                // et évite qu'une particule se teste avec elle-même
                if (j <= i) continue;

                // Calcul de la distance entre particules i et j
                float dxp = x[j] - xi;
                float dyp = y[j] - yi;
                float distSq = dxp * dxp + dyp * dyp;
                float minDist = ri + radius[j];

                // Collision détectée si distance < somme des rayons
                if (distSq < minDist * minDist && distSq > 0.0001f) {
                    float dist = sqrtf(distSq);
                    float overlap = minDist - dist;  // Profondeur d'interpénétration

                    // Vecteur normal unitaire de i vers j
                    float nx = dxp / dist;
                    float ny = dyp / dist;

                    // === ÉTAPE 1 : SÉPARATION DES PARTICULES ===
                    // Les particules s'interpénètrent, on les sépare proportionnellement à leurs masses
                    float mj = mass[j];
                    float totalMass = mi + mj;
                    float p1Ratio = mj / totalMass;  // Particule lourde (j) → particule i bouge plus
                    float p2Ratio = mi / totalMass;  // Particule légère (i) → particule j bouge plus

                    // atomicAdd nécessaire car plusieurs threads peuvent modifier x[i] simultanément
                    atomicAdd(&x[i], -nx * overlap * p1Ratio);
                    atomicAdd(&y[i], -ny * overlap * p1Ratio);
                    atomicAdd(&x[j], nx * overlap * p2Ratio);
                    atomicAdd(&y[j], ny * overlap * p2Ratio);

                    // === ÉTAPE 2 : RÉSOLUTION DE LA COLLISION (IMPULSION) ===
                    // Calcule la vitesse relative de i par rapport à j
                    float relVelX = vxi - vx[j];
                    float relVelY = vyi - vy[j];

                    // Projection de la vitesse relative sur la normale de collision
                    float relVelN = relVelX * nx + relVelY * ny;

                    // Si relVelN > 0, les particules s'approchent → collision à résoudre
                    // Si relVelN <= 0, les particules s'éloignent déjà → pas d'impulsion
                    if (relVelN > 0) {
                        // Formule de l'impulsion élastique : J = (1+e) * v_rel · n / (1/m1 + 1/m2)
                        // Simplifié pour m1, m2 : J / totalMass = (1+e) * v_rel_n
                        float impulse = (1.0f + elasticity) * relVelN / totalMass;

                        // Applique l'impulsion (changement de vitesse)
                        // Particule i : perd de la vitesse dans la direction normale (proportionnel à mj)
                        atomicAdd(&vx[i], -impulse * mj * nx);
                        atomicAdd(&vy[i], -impulse * mj * ny);

                        // Particule j : gagne de la vitesse dans la direction normale (proportionnel à mi)
                        atomicAdd(&vx[j], impulse * mi * nx);
                        atomicAdd(&vy[j], impulse * mi * ny);

                        // Incrémente le compteur de collisions (statistiques)
                        atomicAdd(collisionCount, 1);
                    }
                }
            }
        }
    }
}

// ============================================================================
// FONCTIONS DE LANCEMENT (HOST → DEVICE)
// Ces fonctions sont appelées depuis le CPU pour lancer les kernels sur le GPU
// ============================================================================

/**
 * @brief Lance le kernel d'application des forces (gravité + friction)
 *
 * Calcule le nombre de blocs nécessaires pour traiter toutes les particules
 * et lance le kernel sur le GPU.
 *
 * Configuration: blockSize = 256 threads (8 warps)
 * Nombre de blocs = ceil(count / 256)
 */
void launchApplyForces(DeviceParticles& particles, const DeviceSimParams& params, float dt) {
    int blockSize = Config::CUDA_BLOCK_SIZE;  // 256 threads par bloc
    int numBlocks = (particles.count + blockSize - 1) / blockSize;  // Division arrondie vers le haut

    // Syntaxe CUDA: <<<numBlocs, threadsParBloc>>>
    applyForcesKernel<<<numBlocks, blockSize>>>(
        particles.vx, particles.vy, params.gravity, params.friction, particles.count, dt
    );
}

/**
 * @brief Lance le kernel de force de la souris
 *
 * Ne fait rien si la souris n'est pas dans la zone de rendu (optimisation).
 */
void launchApplyMouseForce(DeviceParticles& particles, const DeviceSimParams& params, float dt) {
    // Optimisation: ne lance pas le kernel si la souris n'est pas active
    if (!params.mouseInArea) return;

    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    applyMouseForceKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy,
        params.mouseX, params.mouseY, params.mouseVX, params.mouseVY,
        params.mouseForceRadius, params.mouseForceStrength,
        params.mouseLeftButton, params.mouseRightButton,
        particles.count, dt
    );
}

/**
 * @brief Lance le kernel de mise à jour des positions
 */
void launchUpdatePositions(DeviceParticles& particles, float dt) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    updatePositionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy, particles.count, dt
    );
}

/**
 * @brief Lance le kernel de collisions avec les murs
 */
void launchWallCollisions(DeviceParticles& particles, const DeviceSimParams& params) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    handleWallCollisionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy, particles.radius,
        params.width, params.height, params.elasticity, particles.count
    );
}

/**
 * @brief Lance le kernel de collisions inter-particules (algorithme de grille spatiale)
 *
 * @param d_collisionCount Pointeur GPU vers le compteur de collisions (réinitialisé avant appel)
 *
 * IMPORTANT: La grille doit être construite AVANT d'appeler cette fonction (buildGrid)
 */
void launchParticleCollisions(DeviceParticles& particles, DeviceGrid& grid,
                              const DeviceSimParams& params, int* d_collisionCount) {
    int blockSize = Config::CUDA_BLOCK_SIZE;
    int numBlocks = (particles.count + blockSize - 1) / blockSize;
    handleParticleCollisionsKernel<<<numBlocks, blockSize>>>(
        particles.x, particles.y, particles.vx, particles.vy,
        particles.radius, particles.mass,
        grid.cellStart, grid.cellEnd, grid.particleIndex,
        grid.gridWidth, grid.gridHeight, grid.cellSize,
        params.elasticity, particles.count, d_collisionCount
    );
}
