# Estudo: incorporando features do Material Maker no TEXGEN

Data: 2026-07-15
Fonte analisada: https://github.com/RodZill4/material-maker (licença **MIT** — porte de código/fórmulas permitido com atribuição a "Rodolphe Suescun and contributors").

## 1. Resumo executivo

O Material Maker (MM) é um editor de materiais PBR procedurais com ~392 nós, rodando 100% em GPU (GLSL compute) sobre o runtime do Godot. O TEXGEN é um gerador de texturas C++ CPU-only (engine gentexture 16-bit + AGG vetorial) com 35 nós, preview via raylib/ImGui e modos headless/export-C.

A boa notícia: **a parte mais valiosa do MM não depende do Godot**. Os nós são arquivos `.mmg` (JSON) contendo fórmulas GLSL puras — matemática de Voronoi, FBM, bricks, warp, blend, SDF — que podem ser portadas diretamente para funções C++ sobre `GenTexture`, no mesmo estilo dos nós já existentes em `extra_generators.hpp`. O que depende de Godot (UI, RenderingDevice, GDScript) o TEXGEN já tem equivalente próprio (ImGui/ImNodes, avaliação CPU).

**Recomendação:** não copiar a arquitetura GPU do MM; portar a *matemática dos nós* e os *conceitos de produto* (export PBR multi-mapa, SDF 2D, subgrafos, parâmetros expostos), em fases, começando por um refactor interno que hoje bloqueia a adição de muitos nós.

## 2. Comparativo de arquitetura

| Aspecto | TEXGEN | Material Maker |
|---|---|---|
| Runtime | C++20, CPU, buffers RGBA 16-bit | Godot 4 / GDScript, GPU (GLSL compute #450) |
| Definição de nó | Classe C++ (`TextureNode`/`CoreNode`) | JSON declarativo (`.mmg`) com template GLSL |
| Nº de nós | 35 | ~392 (321 na biblioteca base) |
| Serialização | JSON próprio (id + slots por nome) | JSON (`.ptex`/`.mmg`), subgrafos aninhados |
| Export | TGA único + header C | PNG/EXR multi-mapa PBR para Godot/Unity/Unreal/Blender/GLTF |
| Formas vetoriais | AGG (rasterização antialiased) | SDF 2D (~55 nós, booleans suaves) |
| 3D | Não | SDF 3D/raymarch, pintura 3D, mesh baking |
| Extensão pelo usuário | Recompilar C++ | Subgrafos viram nós; Custom Shader GLSL |

Referências de código do MM (clone em scratchpad):
- Fórmulas GLSL: `addons/material_maker/nodes/*.mmg` (campos `shader_model.global/instance/code`)
- Funções comuns (rand, hsv): `addons/material_maker/shader_functions.tres`
- Compilador de shader (lógica de grafo→código): `addons/material_maker/engine/nodes/gen_shader.gd`
- Alvos de export PBR: `addons/material_maker/nodes/material.mmg` (`shader_model.exports`)

## 3. Princípio norteador: lib-first

O TEXGEN tem uma proposta que o Material Maker **não** tem: a textura não é só exportada como imagem — ela pode ser **gerada dinamicamente em runtime no jogo**, chamando as funções da `libtexgen` (diretamente ou via header gerado pelo export C). O app/editor é a bancada de experimentação; o produto final é a biblioteca.

Isso impõe uma regra para todo porte de feature do MM:

1. **Todo nó novo nasce como função livre da lib** — em `lib/` (padrão `extra_generators.hpp/.cpp`), operando sobre `GenTexture`, sem nenhuma dependência de UI/raylib/ImGui. Foi assim que Crystal, Bricks, HSCB e Wavelet entraram.
2. **A classe de UI (`TextureNode`) é só um wrapper fino** — widgets de parâmetro + chamada à função da lib. Zero lógica de geração no `work/`.
3. **Cobertura do export C é parte do definition of done** — um nó que o `CExport` não emite quebra a promessa "experimente no editor, use direto no jogo". Hoje já há nós nessa situação (`Bump`, `CoordRemap`, `GlowRect`…) — o refactor da seção 3.1 resolve isso na raiz.
4. **Determinismo e footprint** — as funções da lib devem continuar determinísticas (mesma seed → mesma textura em qualquer máquina) e sem alocações/deps exóticas, pois rodarão dentro de jogos. Nada de I/O ou estado global nos geradores.
5. **API C estável como meta** — conforme o catálogo crescer, vale expor um cabeçalho C puro (`texgen.h` já é o umbrella) para a lib ser consumível de C, engines e bindings.

Consequência para as apostas da Fase 5: **nós declarativos em JSON** só fazem sentido se compilarem para chamadas da lib (o interpretador não pode virar dependência do jogo), e um **backend GPU** seria um acelerador opcional do editor — a lib CPU continua sendo a referência canônica do resultado.

## 3.1 Pré-requisito interno (antes de adicionar muitos nós)

Hoje cada nó novo do TEXGEN precisa ser implementado em **três lugares**: `TextureNode::execute()` (GUI), a cadeia `if/else` de `lib/HeadlessEval.cpp` e a de `work/CExport.cpp`. Há inclusive nós já fora de sincronia (headless não cobre `Cells`, `Bump`, `ColorRemap`, etc.).

**Ação:** migrar a avaliação para o `CoreNodeRegistry` polimórfico (a infra já existe vazia em `lib/CoreNodeRegistry.cpp`), fazendo GUI e headless chamarem o mesmo `CoreNode::execute()`. Sem isso, portar 30+ nós do MM multiplica a dívida por três — e fere diretamente o princípio lib-first da seção 3, já que a versão "de verdade" do nó ficaria no editor em vez da lib.

## 4. Features recomendadas, por prioridade

### Fase 1 — alto valor, baixo esforço (CPU-friendly, encaixa no que já existe)

1. **Export PNG** — `stb_image_write.h` já está vendored e não é usado; trocar/complementar o TGA em `SaveImage()` é trivial.
2. **Nó FBM** (`fbm.mmg`) — value/perlin/perlinabs/cellular com octaves, persistence e "folds". O TEXGEN tem Noise/PerlinNoiseRG2, mas o FBM do MM com variantes cellular é muito mais rico. Matemática 100% portável para CPU.
3. **Voronoi completo** (`voronoi.mmg`, técnica de I. Quilez) — o `Cells` atual não expõe distância a bordas nem UV/cor por célula. O Voronoi do MM devolve 4 saídas (célula, distância a centros, distância a bordas) — base para dezenas de materiais (pedras, escamas, rachaduras).
4. **Bricks avançado** (`bricks.mmg`) — o Bricks atual tem 1 layout; o MM tem running bond, herringbone, basket weave, spanish bond, com saídas extras (cor aleatória por tijolo, UV por tijolo). Reaproveita a estrutura do nó existente.
5. **Nó Blend com 15 modos** (`blend.mmg`) — estender o `Paste` (ou criar `Blend`) com overlay, soft/hard light, burn, dodge, difference, addsub, com opacidade e máscara.
6. **Warp** (`warp.mmg`) — desloca UV pelo gradiente de um heightmap (diferenças finitas). O TEXGEN já tem `CoordRemap`; o Warp é o "irmão" que gera o displacement automaticamente. É um dos nós mais usados do MM.
7. **Colorize/gradient mapping** (`colorize.mmg`) — mapear grayscale→rampa de cores com stops editáveis. O `Gradient` atual só tem 2 cores; um editor de rampa com N stops multiplica o valor de todos os geradores.

### Fase 2 — estrutural, é o que transforma o TEXGEN em ferramenta de *materiais*

8. **Export PBR multi-mapa** — novo nó `Material` com inputs nomeados (albedo, normal, roughness, metallic, height, AO, emission) exportando um conjunto `nome_albedo.png`, `nome_normal.png`, … Modelar segundo `shader_model.exports` do `material.mmg` (inclusive templates para Godot/Unity/Unreal são declarativos e portáveis). **É o maior gap do TEXGEN hoje.**
9. **Família SDF 2D** (`sd*.mmg`, ~55 nós) — círculo, box arredondado, estrela, polígono, arco + operadores boolean/smooth-boolean/morph/repeat. Matemática trivial por pixel em CPU. Complementa o AGG: SDF dá booleans suaves, outline, glow e morphing que rasterização não dá. Sugestão: um tipo de slot "sdf" (grayscale de distância) + nó `SdfShow` para rasterizar.
10. **Normal Map dedicado** (`normal_map.mmg`) — o `Derive` atual gera normais, mas sem strength/formato (OpenGL/DirectX) nem opção de tileable; alinhar com o padrão do MM.
11. **Filtros de acabamento**: Make Tileable, Slope Blur, Bevel, Emboss, Quantize, Tones — todos filtros por pixel/vizinhança portáveis para CPU.
12. **Subgrafos** — agrupar seleção em um nó reutilizável (o copy/paste do `NodeGraph` já serializa subgrafos com remapeamento de IDs; é meio caminho andado) + **parâmetros expostos** (conceito do nó `Remote` do MM: um painel de knobs que controla parâmetros de vários nós).

### Fase 3 — portar os exemplos do Material Maker

O MM traz **44 projetos-exemplo** em `material_maker/examples/*.ptex` (wood, marble, lava, rock, rusted_metal, stone_wall, medieval_wall, mosaic, dry_earth, crocodile_skin, paper, planet…). Portá-los cumpre três papéis de uma vez:

13. **Suíte de validação/regressão** — cada exemplo portado exercita uma combinação real de nós; renderizar headless e comparar com imagem de referência (`ref/`) vira teste automático no `run_tests.bash`.
14. **Conversor `.ptex` → JSON do TEXGEN** — o `.ptex` é JSON (nós com `name/type/parameters` + `connections` por nome/porta), estruturalmente muito próximo do formato do TEXGEN. Um script de conversão com tabela de mapeamento `tipo MM → typeName TEXGEN` automatiza o grosso do porte e **revela objetivamente quais nós ainda faltam** (tipos sem mapeamento = backlog priorizado por frequência de uso).
15. **Galeria de exemplos** — os exemplos portados substituem/expandem os 9 atuais de `examples/`, servindo de material de aprendizado para usuários.

Ordem sugerida: começar pelos exemplos que só usam nós da Fase 1 (bricks, improved_brick, wood, marble usam perlin/voronoi/warp/blend/colorize) e ir liberando os demais conforme a Fase 2 avança. Exemplos 3D (raymarching, 3d_shapes) ficam de fora.

### Fase 4 — UI/UX: ícones SVG, tooltips e hints

Hoje o TEXGEN não tem ícones nem ajuda contextual. O MM resolve isso com dados que podemos reaproveitar:

16. **Tooltips e hints "de graça"** — **371 dos 392 `.mmg` têm `shortdesc`/`longdesc` prontos**, inclusive **por parâmetro** (ex.: Voronoi → param `scale_x`: "The scale along the X axis"). Ao portar cada nó, portar junto as descrições para um campo `description()`/`paramHint()` no `TextureNode`; exibir com `ImGui::SetItemTooltip()` no título do nó, em cada widget de parâmetro e em cada slot de entrada/saída. Também: tooltip com preview ampliado ao passar o mouse no thumbnail do nó.
17. **Ícones SVG** — os ícones do MM são PNGs base64 (previews renderizados), não SVG. Para um set SVG próprio: vendorar **NanoSVG** (header-only, estilo stb, licença zlib) para rasterizar `res/icons/*.svg` em atlas de textura no startup, escalável com o DPI/zoom. Ícones por categoria (gerador/filtro/vetorial/combinador/output) na barra de título dos nós, no menu de criação (add-node) e na toolbar. Alternativa mais simples como primeiro passo: icon font (Font Awesome/Material Symbols via `IconFontCppHeaders`) — menos bonito, porém zero rasterização.
18. **Menu de criação de nós pesquisável** — caixa de busca com filtro por nome/categoria/descrição (o MM tem isso e faz muita diferença com 300+ nós; com o catálogo do TEXGEN crescendo nas fases 1–2, vira necessidade), mostrando ícone + shortdesc em cada entrada.
19. **Onboarding leve** — hint bar no rodapé (dica contextual do que o hover/seleção atual faz), atalhos exibidos nos menus, e um "?" por nó abrindo o `longdesc` completo.

### Fase 5 — apostas maiores (avaliar depois das fases 1–4)

20. **Nós declarativos em JSON** — o insight central do MM: nó = dados (params + expressão), não código. Pela regra lib-first (seção 3), só vale se o nó declarativo **compilar para chamadas da lib / código C** (transpiler), nunca como interpretador embarcado no jogo. Esforço alto.
21. **Backend GPU opcional** — reutilizaria os `.mmg` do MM quase literalmente, mas pela regra lib-first seria apenas um acelerador de preview do editor; a lib CPU 16-bit determinística continua sendo a referência canônica e o caminho de runtime no jogo. Só faz sentido se resolução >1024 / tempo real no editor virarem requisito.
22. **SDF 3D / raymarching, pintura 3D, mesh baking** — fora do escopo natural do TEXGEN (gerador de texturas 2D); não recomendo agora.

## 5. Roadmap sugerido

| Fase | Entregas | Dependências |
|---|---|---|
| 0 | Refactor `CoreNodeRegistry` (execute único p/ GUI+headless+CExport) | — |
| 1 | PNG export; FBM; Voronoi; Bricks v2; Blend; Warp; Colorize | Fase 0 |
| 2 | Nó Material + export PBR multi-mapa; SDF 2D; Normal Map; Make Tileable; subgrafos/params expostos | Fase 1 |
| 3 | Conversor `.ptex`; porte dos 44 exemplos (incremental); testes de regressão por imagem | Fase 1 (parcial), Fase 2 (completo) |
| 4 | Tooltips/hints (descrições do MM); ícones SVG (NanoSVG); menu de nós pesquisável; hint bar | Independente — pode rodar em paralelo às fases 1–3 |
| 5 | Nós declarativos ou backend GPU (decidir com base no uso) | Fases 1–4 |

Observação sobre a Fase 4: os itens de tooltip dependem só de portar as strings junto com cada nó — o ideal é incluí-los no "definition of done" de cada nó portado nas fases 1–2, em vez de deixar para o fim.

## 6. Notas de licença

- Material Maker: MIT (c) 2018-present Rodolphe Suescun and contributors. Ao portar fórmulas/estruturas, manter atribuição no cabeçalho dos arquivos portados e/ou num `THIRD_PARTY.md`.
- Vários shaders do MM derivam de artigos de Inigo Quilez (Voronoi, smooth min de SDF) — citar a origem nos comentários, como o MM faz.
