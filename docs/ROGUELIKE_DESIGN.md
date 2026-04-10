# ROGUELIKE — ASCII Game Design Document
> Plano de referência para GitHub Copilot · Todos os sprites em ASCII 7-bit · Estilo tile-based 2D

---

## ÍNDICE

1. [Personagem Principal](#1-personagem-principal)
2. [Tileset — Mapa e Ambiente](#2-tileset--mapa-e-ambiente)
3. [Layout de Nível — Dungeon Completa](#3-layout-de-nível--dungeon-completa)
4. [Inimigos](#4-inimigos)
5. [Itens de Poder & Consumíveis](#5-itens-de-poder--consumíveis)
6. [Armas & Armas Especiais](#6-armas--armas-especiais)
7. [Habilidades & Efeitos Especiais](#7-habilidades--efeitos-especiais)
8. [HUD & Interface](#8-hud--interface)
9. [Convenções Técnicas para o Copilot](#9-convenções-técnicas-para-o-copilot)

---

## 1. PERSONAGEM PRINCIPAL

### 1.1 Sprite Base — Idle

```
   Θ        ← cabeça / capacete  (glifo alterna por classe)
  (|)       ← torso / armadura
 --|--      ← braços estendidos
   |        ← cintura
  / \       ← pernas / botas
```

### 1.2 Sprites por Classe

```
GUERREIRO          MAGO               ARQUEIRO           LADRÃO
  Θ                 @                  o                  °
  [/|\]             /|\               \|->               */|\
  / \              / \               / \                 / \

capacete metálico   chapéu pontudo    capuz arredondado  capuz baixo
espada na direita   cajado na mão     arco tenso         adaga oculta
escudo na esquerda  manto arrastando  carcaj nas costas  postura baixa
```

### 1.3 Estados de Animação

```
IDLE          ANDANDO-1     ANDANDO-2     ANDANDO-3     ANDANDO-4
  Θ              Θ             Θ             Θ             Θ
 (|)            (|)           (|)           (|)           (|)
--|--          --|--         --|--         --|--         --|--
  |              |            / \           \ /            |
 / \            / \          /   \         /   \          / \
(frames: 180ms cada — loop contínuo enquanto dx != 0)

ATACANDO-1    ATACANDO-2    ATACANDO-3    VOLTA
  Θ             Θ             Θ             Θ
 (|)           (|)          (|)->         (|)
--|--          --|--\        --|--         --|--
  |              |            |             |
 / \            / \          / \           / \
(frame 2: braço recua · frame 3: impacto — spawn hitbox · frame 4: retorno)

DANO          MORTO         DEFENDENDO    AGACHADO      PULANDO
  @             x            [Θ]            Θ            \Θ/
 \|/          --+--         [/|\]          _|_           \|/
  | \           |            / \           / \           / \
(@ = stunface) (deitado)    (escudo ativo) (menor hitbox)(aéreo)
```

### 1.4 Efeitos de Status sobre o Sprite

```
ENVENENADO     EM CHAMAS      CONGELADO      AMALDIÇOADO    INVISÍVEL
  ö              Θ*             [Θ]            Θ̈              .
 /|\~           /|\^           =|=*          (/|\)            .
 / \           / \            [/ \]          / \~            ...
(~ = vapores)  (* = faíscas)  ([] = gelo)   (() = sombra)   (... = rastro)
```

---

## 2. TILESET — MAPA E AMBIENTE

### 2.1 Tiles Estruturais

```
PAREDE SÓLIDA      PAREDE COM RACHADURA   PAREDE + TOCHA
████████████       ████/███████           ████)f(████
█  solid  █        █  cracked  █          █  lit  █
████████████       ████████████           ████████████

PISO PEDRA         PISO TERRA             PISO LAVA
............       ,,,,,,,,,,,,           ≈≈≈≈≈≈≈≈≈≈≈≈
.  stone  .        ,   dirt   ,           ≈  lava  ≈
............       ,,,,,,,,,,,,           ≈≈≈≈≈≈≈≈≈≈≈≈

PISO ÁGUA          PISO ABISMO            PISO ICE
~~~~~~~~~~~~       ············           ############
~  water  ~        .  void  .            #   ice   #
~~~~~~~~~~~~       ············           ############
(causa slow)       (causa morte)          (causa slide)
```

### 2.2 Tiles de Transição e Bordas

```
CANTO SUP-ESQ    CANTO SUP-DIR    CANTO INF-ESQ    CANTO INF-DIR
╔════════         ════════╗         ╚════════         ════════╝

BORDA TOPO       BORDA LATERAL    CRUZAMENTO       T-JUNCTION
════════════     ║               ╬                ╦
                 ║               ╬                ╩
```

### 2.3 Objetos de Decoração

```
TOCHA NA PAREDE    BARRIL             CAIXOTE           CAVEIRA NO CHÃO
  ) f (              ___              ___                   _
  )|f|(             (   )            [   ]                 (@)
  )|f|(             |___|            |___|                 / \
   ~~~             (chama anima)    (quebrável)           (derrubado)

COLUNA             ALTAR             PORTAL FECHADO      PORTAL ABERTO
  |||               _^_               ╔═══╗               ╔≈≈≈╗
 =|||=             /___\              ║   ║               ║≈≈≈║
  |||             |_____|             ╚═══╝               ╚≈≈≈╝
 _|||_            (ativa buff)       (trancado)           (brilha/pulsa)

ESCADA DESCENDO    ESCADA SUBINDO    TESOURO FECHADO     TESOURO ABERTO
   <<<                >>>              _____               ___
  < < <              > > >            |     |             |   |
  <   <              >   >            |_____|             |___|
(próximo nível)   (nível anterior)  ($ ao abrir)         (coletado)

TEIA DE ARANHA     COGUMELO          OSSOS              POÇA DE SANGUE
   *  *  *            ^               \/\/               . . .
  *  *  *            (^)              ----              . . . .
   * * *             | |             /\/\/               . . .
(slow + damage)   (venenoso)        (decorativo)        (decorativo)

PORTA FECHADA      PORTA ABERTA      PORTA TRANCADA      ALAVANCA
  |====|              |    |            |====|               _|
  |====|              |    |            |@===|              / |
  |====|              |    |            |====|             /__|
  ------              ------            ------
```

### 2.4 Armadilhas

```
ESPINHOS (retraídos)   ESPINHOS (ativos)    PLATAFORMA FALSA
    .  .  .               /\/\/\/            ~~~~~~~~~~~~
   .  .  .               / \ / \             ~~~~~~~~~~~~
    .  .  .             /   V   \            (cai ao pisar)

DARDO NA PAREDE       PISO ELÉTRICO         ARMADILHA DE FOGO
  ║--->               ####_####            |     |
  ║                   #_sparks_#            |  ^  |
  ║                   #########             | )|( |
(dispara ao pisar)   (dano periódico)       (dispara ao cruzar)
```

---

## 3. LAYOUT DE NÍVEL — DUNGEON COMPLETA

### 3.1 Mapa de Exemplo — Nível 1 (30×20 tiles)

```
╔══════════════════════════════╗
║████████████████████████████║
║█............█....██.....█..║
║█..Θ.........█....██..E..█..║  ← player spawn (Θ), inimigo (E)
║█............+....██.....█..║  ← porta (+)
║█............█....██.....╗..║
║█████████████╗....╔████████║
║............█║....║█.......║
║..f...[T]...█║....║█..≈≈≈.║  ← tocha (f), tesouro ([T]), água (≈)
║............█║....║█.......║
║███████+████║....║█.......║
║█..S........╚════╝█...E...║  ← S = loja, E = inimigo
║█...████████████████......║
║█...█................████║
║█...█...E...E........█..║
║█...█................█..║
║█...╚════════════════╝..║
║█.......................█║
║██████████████████████████║
╚══════════════════════════════╝

LEGENDA:
  Θ  = player spawn     E  = enemy spawn     S  = shop
  +  = door             f  = torch           ≈  = water
 [T] = treasure chest   █  = wall            .  = floor
  ╔╗╚╝╠╣╦╩╬ = corners/junctions
```

### 3.2 Sala de Boss (15×12 tiles)

```
  ╔═══════════════╗
  ║███████████████║
  ║█.............█║
  ║█.....B.......█║  ← B = Boss (centro da sala)
  ║█.............█║
  ║█..^..^..^....█║  ← ^ = espinhos decorativos
  ║█.............█║
  ║█.............█║
  ║█.....Θ.......█║  ← player enter point
  ║█.............█║
  ║███████████████║
  ╚═════+═════════╝  ← + = porta de entrada (tranca ao entrar)
```

### 3.3 Sala de Loja (10×8 tiles)

```
  ╔══════════╗
  ║██████████║
  ║█........█║
  ║█.[$][$].█║  ← itens à venda
  ║█........█║
  ║█..NPC...█║  ← vendedor
  ║█........█║
  ║██████████║
  ╚════+═════╝
```

---

## 4. INIMIGOS

### 4.1 Inimigos Comuns

```
GOBLIN             ESQUELETO          ZUMBI              MORCEGO
  ^                   o                 @                  /\
 (o)                 /|\               /|~                (  )
  |                  | |              / \                  \/
 / \                / \                                   ><><
HP:20 ATK:5        HP:15 ATK:7        HP:25 ATK:4        HP:10 ATK:3
MOV:fast           MOV:medium         MOV:slow+swarm      MOV:flying
DROP:coin          DROP:bone          DROP:rotten_meat    DROP:wing

SLIME              RATO               ARANHA              FUNGO
  ___                ~                  *                   ^
 /   \              ( )                (*)                 (^)
|  ^^ |              V                 /|                   |
 \___/                                / \
HP:12 ATK:3        HP:8  ATK:2        HP:18 ATK:6         HP:30 ATK:2
MOV:jump           MOV:erratic        MOV:wall_climb       MOV:stationary
DROP:slime         DROP:nothing       DROP:web             DROP:spore+poison
```

### 4.2 Inimigos de Elite

```
CAVALEIRO SOMBRIO          NECROMANTE               GOLEM DE PEDRA
      Θ                        ô                        ███
    [/|\]                     /|\                      █ ° °█
     / \                      | \                      █  _ █
                               ~                       █/ \█
HP:80  ATK:18                HP:50 ATK:12             HP:150 ATK:25
MOV:medium+block             MOV:ranged+summon         MOV:slow+stomp
DROP:dark_sword              DROP:dark_tome            DROP:stone_core
SKILL: "Golpe Escudo"        SKILL: "Invocar Mortos"   SKILL: "Terremoto"

ASSASSINO                  BRUXA                    DRAGÃO JOVEM
   °                          ö                          /\
  /|.                        /|\                        / \/\
  /|_                        | \                       /      \
   ~                          ~                        \  V  /
HP:45 ATK:22                HP:40 ATK:15               HP:120 ATK:30
MOV:teleport+backstab       MOV:fly+curse              MOV:fly+breath
DROP:poison_dagger          DROP:hex_staff             DROP:dragon_scale
SKILL: "Evasão"             SKILL: "Maldição"          SKILL: "Baforada"
```

### 4.3 Bosses

```
BOSS LVL 1 — O GUARDIÃO DA CRIPTA       BOSS LVL 2 — LICHE REI
         ___                                      ☠
        /   \                                  ╔/═\╗
       | o o |                                ╔╝   ╚╗
        \   /                                 ║     ║
         | |                                  ╚══╦══╝
        /   \                                     |
       |     |                                   / \
       |_____|
HP:300  ATK:35  DEF:20                  HP:500  ATK:45  DEF:10
PHASE 1: corpo-a-corpo + slam           PHASE 1: magia + escudos
PHASE 2 (HP<150): invoca 2 esqueletos   PHASE 2 (HP<250): invoca liches
PHASE 3 (HP<50): fúria — ATK x2        PHASE 3 (HP<100): teleporte frenético
WEAKNESS: fogo                          WEAKNESS: luz sagrada
DROP: Clave da Cripta + 200g            DROP: Coroa do Liche + 500g + portal

BOSS LVL 3 — BEHEMOTH DO ABISMO
           ___________
          /     |     \
      /   0     0   \
     |      ___      |
     |     /___\     |
     |_____/___\_____|
       /  |  \
      /   V   \
     /   / \   \
HP:800  ATK:60  DEF:35
PHASE 1: stomps + shockwave (AoE)
PHASE 2 (HP<400): invoca portal de lava
PHASE 3 (HP<200): divide em 2 mini-behemoths (HP:200 cada)
WEAKNESS: gelo + raio
DROP: Coração do Abismo (lendário) + 1000g
```

### 4.4 Comportamento de IA dos Inimigos

```
ESTADO DE IA:     IDLE → PATROL → CHASE → ATTACK → RETREAT
                    |        |        |       |         |
                  dorme   ronda    persegue  agride  recua (HP<20%)

PATROL PATH (exemplo):
  . → . → . → .
  ↑             ↓
  . ← . ← . ← .
  (loop até detectar player no raio de visão)

RAIO DE VISÃO (tiles):
  Goblin      : 4
  Esqueleto   : 3
  Assassino   : 6 (cone traseiro também)
  Liche       : 8 (linha de visão por magia)
  Boss        : sala inteira
```

---

## 5. ITENS DE PODER & CONSUMÍVEIS

### 5.1 Poções

```
POÇÃO DE VIDA          POÇÃO DE MANA          POÇÃO DE FORÇA
      _                      _                       _
     | |                    | |                     | |
    (   )                  (   )                   (   )
     | |                    | |                     | |
     |_|                    |_|                     |_|
   [vermelho]             [azul]                  [laranja]
HEAL: +30 HP             +20 MP                ATK +10 por 30s

POÇÃO DE VELOCIDADE    POÇÃO DE INVISIBILIDADE  ÉLIX. DA RESSURREIÇÃO
      _                       _                       *_*
     |~|                     |.|                    (   )
    ( ~ )                   ( . )                    | |
     |~|                     |.|                     |_|
     |_|                     |_|                   [dourado]
SPD +50% por 20s           Invisível 15s          Revive com 50% HP
```

### 5.2 Pergaminhos

```
PERGAMINHO DE TELEPORTE   MAPA REVELADO         IDENTIF. MÁGICA
       ___                    ___                    ___
      /   \                  /   \                  /   \
     | >>> |                | ::: |                | ??? |
     | >>> |                | ::: |                | !!! |
      \___/                  \___/                  \___/
Teleporta aleatório        Revela mapa completo   Identifica 1 item

PERGAMINHO DE FOGO         PERGAMINHO DE GELO     PERGAMINHO DE CURA
       ___                    ___                    ___
      /   \                  /   \                  /   \
     | ))) |                | *** |                | +++ |
     | ))) |                | *** |                | +++ |
      \___/                  \___/                  \___/
AoE fogo 3x3 tiles         AoE gelo 3x3 tiles     Cura todos aliados
```

### 5.3 Relíquias Passivas (Rogue Items)

```
AMULETO DA SOMBRA          ANEL DO TITÃ           PEDRA RÚNICA
       ___                       O                   /___\
      / o \                     |||                 | ᚱᚢᚾ |
     |     |                     O                  |_____|
      \___/
+15% evasão               +20 HP máx              Habilidade aleatória

CORAÇÃO DE CRISTAL         OLHO DO DEMÔNIO        PENA DE GRIFO
      _*_                     ( )                     ,
     / * \                   (   )                   /|
    |  *  |                   ( )                   //
     \*_*/                                          /
+1 vida extra (1x)          Vê inimigos no mapa    +30% velocidade de pulo
```

---

## 6. ARMAS & ARMAS ESPECIAIS

### 6.1 Armas Comuns

```
ADAGA              ESPADA CURTA       ESPADA LONGA       MACHADO
    /                  /                   /               _
   /                  /=                  /=              / |
  *                  *=                  *=             /=  |
                                                        *   |
ATK:8 SPD:fast     ATK:12 SPD:med     ATK:18 SPD:slow  ATK:20 SPD:slow
RANGE:1 tile       RANGE:1 tile       RANGE:1 tile      RANGE:1 tile
CRIT:25%           CRIT:15%           CRIT:10%          CRIT:20%

ARCO CURTO         ARCO LONGO         BESTA              CAJADO
   )                  )                  T                 |*|
  )=-->               )=-->              T-->               |
   )                  )                  T                  |
ATK:10 SPD:med     ATK:14 SPD:slow    ATK:22 SPD:vslow  ATK:8+magia
RANGE:5 tiles      RANGE:8 tiles      RANGE:7 tiles     RANGE:4 tiles
AMMO:arrow         AMMO:arrow         AMMO:bolt         MANA:5/cast
```

### 6.2 Armas Raras

```
ESPADA FLAMEJANTE                 LANÇA ELÉTRICA
     /))                              |
    /))=                             |#|
   *))=                             |#|---->
                                     |#|
ATK:25 + burn(5/s por 3s)          ATK:20 + chain (2 inimigos extras)
ANIMAÇÃO: faíscas laranja no sprite ANIMAÇÃO: raio azul ao atacar
SPAWN: boss drop / loja rara        SPAWN: gerado aleatório (rare)

GLAIVE DAS SOMBRAS                MARTELO DO TROVÃO
    /\                                 |
   /  \                                |
  / /\ \                            [===]
 *      \
ATK:30 RANGE:2 tiles                ATK:35 + AoE stun (1s)
+15% dano por atrás (backstab)      RANGE:1 tile (área de efeito)
ANIMAÇÃO: rastro escuro             ANIMAÇÃO: raio ao impacto

FOICE DO NECROMANTE               ARCO DO VENTO
      _                                )
    _/ \                              )~-->>
   /    \                            )
  *      |
ATK:28 + dreno (rouba 8 HP)         ATK:18 + knockback (2 tiles)
ANIMAÇÃO: nuvem roxa ao acertar     ANIMAÇÃO: rastro branco na flecha
```

### 6.3 Armas Lendárias (Tier S)

```
LÂMINA DO CAOS                    BASTÃO DO ARCANO SUPREMO
   /~\                                 |Ω|
  /~=~\                               |Ω|
 *~====                               |Ω|
  \~~/                                 |
ATK:50 + efeito aleatório por hit   ATK:15 + spell gratuito a cada 5 hits
(fogo / gelo / raio / veneno)       MANA:0 para o spell bônus
SPAWN: boss final                   SPAWN: sala secreta lv3

PUNHAIS GÊMEOS DA LUA               ARCO ASTRAL
   /   /                                 )
  /   /                                 )===*>>
 * + *                                   )
(dual wield — ataca 2x por turno)    ATK:40 + pierce (atravessa inimigos)
ATK: 20+20 SPD: ultrafast            RANGE: tela inteira
CRIT: 40%                            AMMO: infinito (usa MP 3/tiro)
SPAWN: ladrão lv10                   SPAWN: boss lv2 drop
```

---

## 7. HABILIDADES & EFEITOS ESPECIAIS

### 7.1 Habilidades por Classe

```
GUERREIRO — HABILIDADES
┌─────────────────────────────────────────────────────────────┐
│ GOLPE BRUTAL          │ GRITO DE GUERRA       │ ESCUDO TOTAL │
│    Θ                  │    \o/                │    [Θ]       │
│  /|\>*<               │    /|\                │   [/|\]      │
│  / \                  │   / \                 │   [/ \]      │
│ ATK x2 · CD:3turns    │ ATK+30% 3 turns       │ DEF x3 1turn │
│ Custo: 10 MP          │ Custo: 8 MP           │ Custo: 6 MP  │
└─────────────────────────────────────────────────────────────┘

MAGO — HABILIDADES
┌─────────────────────────────────────────────────────────────┐
│ BOLA DE FOGO          │ BLIZZARD              │ RAIO         │
│    ô                  │    ô                  │    ô         │
│  *)))>                │  *~~~>                │  *===>>      │
│                       │  *~~~>                │              │
│ AoE 2x2 · 30 dmg      │ AoE 4x4 · 15 + slow   │ Single 45dmg │
│ Custo: 15 MP          │ Custo: 25 MP          │ Custo: 20 MP │
└─────────────────────────────────────────────────────────────┘

ARQUEIRO — HABILIDADES
┌─────────────────────────────────────────────────────────────┐
│ CHUVA DE FLECHAS      │ FLECHA PERFURANTE     │ ARMADILHA    │
│    o                  │    o                  │     o        │
│  \|====>>             │  \|-->-->-->          │  \|->  [*]   │
│  ||||||||             │                       │              │
│ AoE coluna · 12 dmg   │ Pierce 3 inimigos     │ Trap no tile │
│ Custo: 18 MP          │ Custo: 12 MP          │ Custo: 8 MP  │
└─────────────────────────────────────────────────────────────┘

LADRÃO — HABILIDADES
┌─────────────────────────────────────────────────────────────┐
│ EVASÃO                │ VENENO NA LÂMINA      │ FUMAÇA       │
│   ° .                 │    °                  │    .   .     │
│  /|.  .               │  /|.*->               │  . ° .   .   │
│  ___                  │  /|_                  │   /|.        │
│ Invulnerável 1 turn   │ +15 veneno/3s         │ Escapa combate│
│ Custo: 10 MP          │ Custo: 6 MP           │ Custo: 12 MP │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 Efeitos Visuais de Habilidades

```
FOGO (projetil)        GELO (area)            RAIO (instantâneo)
  o)))>                ~~~*~~~                o=====>E
  propagação:          *~~~~~*                (frame único — flash)
  . o))) >             ~*~~~*~
  . . o))) >           ~~~*~~~

CURA (self)            ESCUDO (self)          TELEPORTE
   +                     ###                   *poof*
  +++                   #   #                  o → . → Θ
   +                     ###                  (fade out/in)
 (pulsa 3x)             (anel ao redor)

VENENO (DoT)           PARALISIA (CC)         MALDIÇÃO (debuff)
   ~                      !!!                    ~ψ~
  ~~o~~                   /o\                   /o\
   ~                     _|_                   _|_~
 (nuvem verde)           (estrelas)            (aura roxa)
```

### 7.3 Partículas & VFX por Tile

```
IMPACTO EM PAREDE:   * . .       CRÍTICO:    !!!  *
                    . * .                   *  Θ  *
                     . . *                  !!!  *

PICKUP DE ITEM:     ↑ $ ↑       LEVEL UP:  ★★★
                     \$/                   \o/
                      Θ                    /|\

MORTE DE INIMIGO:   x . .       EXPLOSÃO:  *   *
                   . x .                  * ))) *
                    . . x                  *   *
```

---

## 8. HUD & INTERFACE

### 8.1 HUD In-Game (topo da tela)

```
╔═══════════════════════════════════════════════════════════╗
║ [Θ] HP: ████████░░  80/100   MP: ████░░░░  40/80        ║
║      EXP: ███████░░░░░░░  350/500     LVL: 5            ║
║ GOLD: 220$    KILLS: 13    FLOOR: B2    TIME: 04:32      ║
╚═══════════════════════════════════════════════════════════╝
```

### 8.2 Inventário (pause screen)

```
╔══════════════════════════════════════════╗
║           INVENTÁRIO — Rick              ║
╠═══════════╦══════════════════════════════╣
║  ARMA     ║  [/] Espada Flamejante       ║
║  ESCUDO   ║  [[] Escudo de Ferro         ║
║  ELMO     ║  [Θ] Capacete de Aço         ║
║  ARMADURA ║  [H] Couraça de Couro        ║
╠═══════════╩══════════════════════════════╣
║  BOLSA DE ITENS (12 slots):              ║
║  [P][P][P][M][M][∞][ ][ ][ ][ ][ ][ ]    ║
║   ↑   ↑   ↑   ↑   ↑   ↑                  ║
║  HP HP HP MP MP Relíquia                 ║
╠══════════════════════════════════════════╣
║  HABILIDADES (hotkeys 1–4):              ║
║  [1]Golpe [2]Grito [3]Escudo [4]--       ║
╚══════════════════════════════════════════╝
```

### 8.3 Tela de Game Over / Victory

```
GAME OVER                          VITÓRIA
  ╔═══════════════╗                ╔═══════════════╗
  ║   x       x   ║                ║ ★   ★   ★   ★ ║
  ║    \     /    ║                ║               ║
  ║     -----     ║                ║   YOU WIN!    ║
  ║               ║                ║               ║
  ║   YOU DIED    ║                ║  \o/ hooray   ║
  ║               ║                ║  /|\          ║
  ║ [R] Reiniciar ║                ║  / \          ║
  ║ [M] Menu      ║                ║               ║
  ╚═══════════════╝                ╚═══════════════╝
```

---

## 9. CONVENÇÕES TÉCNICAS PARA O COPILOT

### 9.1 Sistema de Coordenadas

```
(0,0) ──────────── x →
  │   ┌──────────────┐
  │   │  TILE MAP    │
  y   │  tile[y][x]  │
  ↓   │              │
      └──────────────┘

TILE_SIZE = 16px   (renderização em pixel-art / canvas)
MAP_WIDTH  = variável (mínimo 20 tiles)
MAP_HEIGHT = variável (mínimo 15 tiles)
```

### 9.2 Estrutura de Dados dos Sprites

```c
/* Cada sprite é um array de strings — uma por linha do desenho */
typedef struct {
    const char *lines[MAX_SPRITE_LINES];
    int         width;
    int         height;
    int         origin_x;   /* coluna do tile de referência */
    int         origin_y;   /* linha do tile de referência  */
} AsciiSprite;

/* Exemplo de instanciação */
AsciiSprite player_idle = {
    .lines    = { "  Θ  ", " (|) ", "--|--", "  |  ", " / \\ " },
    .width    = 5,
    .height   = 5,
    .origin_x = 2,
    .origin_y = 4,
};
```

### 9.3 Mapa de Tiles — Enum Recomendado

```c
typedef enum {
    TILE_EMPTY       = ' ',
    TILE_WALL        = '#',   /* wall sólida */
    TILE_FLOOR       = '.',   /* piso pedra  */
    TILE_DOOR        = '+',   /* porta       */
    TILE_DOOR_OPEN   = '/',   /* porta aberta*/
    TILE_STAIRS_DOWN = '>',   /* escada ↓    */
    TILE_STAIRS_UP   = '<',   /* escada ↑    */
    TILE_WATER       = '~',   /* água        */
    TILE_LAVA        = '=',   /* lava        */
    TILE_TRAP        = '^',   /* armadilha   */
    TILE_CHEST       = '$',   /* tesouro     */
    TILE_TORCH       = 'f',   /* tocha       */
    TILE_PORTAL      = 'O',   /* portal      */
} TileType;
```

### 9.4 Diretrizes de Geração de Código para o Copilot

```
REGRAS DE DESIGN:
  1. Cada sprite deve ter exatamente um char de "hitbox principal"
     (ex: 'Θ' para player, 'E' para enemy) no tile de origin_x/origin_y.

  2. Animações são arrays circulares de AsciiSprite —
     frame = (tick / frame_duration) % num_frames

  3. Colisão é feita no grid de tiles, não no pixel —
     checar tile[ny][nx] antes de mover entidade.

  4. Spawn de inimigos: reservar tiles marcados com 'E' no mapa,
     substituir por TILE_FLOOR após instanciar o inimigo.

  5. Drops de itens: ao matar inimigo, chamar roll_drop(enemy->loot_table)
     e posicionar o sprite do item no tile onde o inimigo estava.

  6. Efeitos de status (veneno, fogo, gelo): aplicar como flags na struct
     Entity — bit field recomendado para performance.
     typedef uint8_t StatusFlags;
     #define STATUS_POISON  (1 << 0)
     #define STATUS_BURN    (1 << 1)
     #define STATUS_FREEZE  (1 << 2)
     #define STATUS_STUN    (1 << 3)
     #define STATUS_CURSE   (1 << 4)
     #define STATUS_INVIS   (1 << 5)

  7. O sprite do personagem deve trocar o glifo da cabeça de acordo com
     a classe ativa: 'o' base, 'Θ' guerreiro, 'ô' mago, '°' ladrão.

  8. HUD renderiza separado do tile map — layer de UI sobreposto.
```

### 9.5 Paleta de Cores Recomendada (ANSI / ncurses)

```
ELEMENTO          COR ANSI         COR HEX (para canvas)
─────────────────────────────────────────────────────────
Parede            BRANCO INTENSO   #C0C0C0
Piso              CINZA ESCURO     #404040
Player            AMARELO          #FFD700
Inimigo Comum     VERMELHO         #CC4444
Inimigo Elite     MAGENTA          #CC44CC
Boss              VERMELHO INTENSO #FF0000
Item HP           VERMELHO CLARO   #FF6666
Item MP           AZUL CLARO       #6666FF
Item Ouro         AMARELO          #FFD700
Arma Comum        CIANO            #44CCCC
Arma Rara         AZUL             #4444FF
Arma Lendária     DOURADO          #FFA500
Tocha / Fogo      AMARELO INTENSO  #FFAA00
Água              AZUL ESCURO      #224488
Lava              LARANJA          #FF6600
Veneno            VERDE            #44CC44
Gelo              CIANO CLARO      #AAFFFF
HUD Texto         BRANCO           #FFFFFF
HUD Barra HP      VERMELHO         #FF2222
HUD Barra MP      AZUL             #2222FF
HUD Barra EXP     VERDE            #22FF22
```

---

*Compatível com: C/C++ (ncurses)
